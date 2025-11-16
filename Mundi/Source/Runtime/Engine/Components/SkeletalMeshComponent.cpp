#include "pch.h"
#include "SkeletalMeshComponent.h"
#include "AnimSingleNodeInstance.h"
#include "AnimationAsset.h"
#include "AnimSequence.h"

USkeletalMeshComponent::USkeletalMeshComponent()
{
    // 테스트용 기본 메시 설정
    SetSkeletalMesh(GDataDir + "/Test.fbx"); 
}


void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    
    if (!SkeletalMesh) { return; }

    // 애니메이션 활성화 시 AnimScriptInstance 업데이트
    if (bEnableAnimation && AnimScriptInstance)
    {
        FPoseContext OutPose;
        AnimScriptInstance->EvaluateAnimationPose(DeltaTime, OutPose);

        // 애니메이션에서 계산된 포즈를 CurrentLocalSpacePose에 적용
        if (OutPose.LocalTransforms.Num() == CurrentLocalSpacePose.Num())
        {
            CurrentLocalSpacePose = OutPose.LocalTransforms;
            ForceRecomputePose();
        }
    }
    
    //// FOR TEST ////
    // 아래 테스트 코드는 주석 처리하거나 제거하세요
    // 애니메이션 시스템과 충돌할 수 있습니다
    /*
    constexpr int32 TEST_BONE_INDEX = 2;
    
    if (!bIsInitialized)
    {
        TestBoneBasePose = CurrentLocalSpacePose[TEST_BONE_INDEX];
        bIsInitialized = true;
    }
    TestTime += DeltaTime;

    float Angle = sinf(TestTime * 2.f);
    FQuat TestRotation = FQuat::FromAxisAngle(FVector(1.f, 0.f, 0.f), Angle);
    TestRotation.Normalize();

    FTransform NewLocalPose = TestBoneBasePose;
    NewLocalPose.Rotation = TestRotation * TestBoneBasePose.Rotation;
    
    SetBoneLocalTransform(TEST_BONE_INDEX, NewLocalPose);
    */
    //// FOR TEST ////
}

void USkeletalMeshComponent::SetSkeletalMesh(const FString& PathFileName)
{
    Super::SetSkeletalMesh(PathFileName);

    if (SkeletalMesh && SkeletalMesh->GetSkeletalMeshData())
    {
        const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
        const int32 NumBones = Skeleton.Bones.Num();

        CurrentLocalSpacePose.SetNum(NumBones);
        CurrentComponentSpacePose.SetNum(NumBones);
        TempFinalSkinningMatrices.SetNum(NumBones);
        TempFinalSkinningNormalMatrices.SetNum(NumBones);

        for (int32 i = 0; i < NumBones; ++i)
        {
            const FBone& ThisBone = Skeleton.Bones[i];
            const int32 ParentIndex = ThisBone.ParentIndex;
            FMatrix LocalBindMatrix;

            if (ParentIndex == -1) // 루트 본
            {
                LocalBindMatrix = ThisBone.BindPose;
            }
            else // 자식 본
            {
                const FMatrix& ParentInverseBindPose = Skeleton.Bones[ParentIndex].InverseBindPose;
                LocalBindMatrix = ThisBone.BindPose * ParentInverseBindPose;
            }
            // 계산된 로컬 행렬을 로컬 트랜스폼으로 변환
            CurrentLocalSpacePose[i] = FTransform(LocalBindMatrix); 
        }
        
        ForceRecomputePose();

		if (bEnableAnimation)
		{
			bool bInitialized = InitializeAnimScriptInstance();
			if (bInitialized)
			{
				if (AnimationMode == EAnimationMode::AnimationSingleNode)
				{
					UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
					if (SingleNodeInstance)
					{
						UAnimSequence* DefaultAnimAsset = UResourceManager::GetInstance().Load<UAnimSequence>(PathFileName);
						SingleNodeInstance->SetAnimationAsset(DefaultAnimAsset, true);
					}
				}
				// TODO: 다른 모드일 경우 처리 필요
			}
			else
			{
				UE_LOG("SetSkeletalMesh: Failed to initialize AnimScriptInstance");
			}
		}
    }
    else
    {
        // 메시 로드 실패 시 버퍼 비우기
        CurrentLocalSpacePose.Empty();
        CurrentComponentSpacePose.Empty();
        TempFinalSkinningMatrices.Empty();
        TempFinalSkinningNormalMatrices.Empty();
    }
}

void USkeletalMeshComponent::SetBoneLocalTransform(int32 BoneIndex, const FTransform& NewLocalTransform)
{
    if (CurrentLocalSpacePose.Num() > BoneIndex)
    {
        CurrentLocalSpacePose[BoneIndex] = NewLocalTransform;
        ForceRecomputePose();
    }
}

void USkeletalMeshComponent::SetBoneWorldTransform(int32 BoneIndex, const FTransform& NewWorldTransform)
{
    if (BoneIndex < 0 || BoneIndex >= CurrentLocalSpacePose.Num())
        return;

    const int32 ParentIndex = SkeletalMesh->GetSkeleton()->Bones[BoneIndex].ParentIndex;

    const FTransform& ParentWorldTransform = GetBoneWorldTransform(ParentIndex);
    FTransform DesiredLocal = ParentWorldTransform.GetRelativeTransform(NewWorldTransform);

    SetBoneLocalTransform(BoneIndex, DesiredLocal);
}


FTransform USkeletalMeshComponent::GetBoneLocalTransform(int32 BoneIndex) const
{
    if (CurrentLocalSpacePose.Num() > BoneIndex)
    {
        return CurrentLocalSpacePose[BoneIndex];
    }
    return FTransform();
}

FTransform USkeletalMeshComponent::GetBoneWorldTransform(int32 BoneIndex)
{
    if (CurrentLocalSpacePose.Num() > BoneIndex && BoneIndex >= 0)
    {
        // 뼈의 컴포넌트 공간 트랜스폼 * 컴포넌트의 월드 트랜스폼
        return GetWorldTransform().GetWorldTransform(CurrentComponentSpacePose[BoneIndex]);
    }
    return GetWorldTransform(); // 실패 시 컴포넌트 위치 반환
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const
{
	return Cast<class UAnimSingleNodeInstance>(AnimScriptInstance);
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode InAnimationMode, bool bForceInitAnimScriptInstance)
{
	if (!bEnableAnimation)
	{
		UE_LOG("SetAnimationMode: Animation is currently disabled");
		return;
	}

	const bool bNeedChange = AnimationMode != InAnimationMode;
	if (bNeedChange)
	{
		AnimationMode = InAnimationMode;
		ClearAnimScriptInstance();
	}

	// 아래는 UE의 주석
	// when mode is swapped, make sure to reinitialize
	// even if it was same mode, this was due to users who wants to use BP construction script to do this
	// if you use it in the construction script, it gets serialized, but it never instantiate. 
	if (GetSkeletalMesh() != nullptr && (bNeedChange || (AnimationMode == EAnimationMode::AnimationBlueprint && bForceInitAnimScriptInstance)))
	{
		bool bInitialized = InitializeAnimScriptInstance();
		if(!bInitialized)
		{
			UE_LOG("SetAnimationMode: Failed to initialize AnimScriptInstance");
		}
	}
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimToPlay)
{
	if (!bEnableAnimation)
	{
		UE_LOG("SetAnimation: Animation is currently disabled");
		return;
	}

	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->SetAnimationAsset(NewAnimToPlay, false);
		SingleNodeInstance->SetPlaying(false);
	}
	else if (AnimScriptInstance != nullptr)
	{
		UE_LOG("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset");
	}
}

void USkeletalMeshComponent::Play(bool bLooping)
{
	if (!bEnableAnimation)
	{
		UE_LOG("Play: Animation is currently disabled");
		return;
	}

	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->SetPlaying(true);
		SingleNodeInstance->SetLooping(bLooping);
	}
	else if (AnimScriptInstance != nullptr)
	{
		UE_LOG("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset");
	}
}

void USkeletalMeshComponent::PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping)
{
	if (!bEnableAnimation)
	{
		UE_LOG("PlayAnimation: Animation is currently disabled");
		return;
	}

	SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SetAnimation(NewAnimToPlay);
	Play(bLooping);
}

void USkeletalMeshComponent::ForceRecomputePose()
{
    if (!SkeletalMesh) { return; } 

    // LocalSpace -> ComponentSpace 계산
    UpdateComponentSpaceTransforms();
    // ComponentSpace -> Final Skinning Matrices 계산
    UpdateFinalSkinningMatrices();
    UpdateSkinningMatrices(TempFinalSkinningMatrices, TempFinalSkinningNormalMatrices);
    PerformCpuSkinning();
}

void USkeletalMeshComponent::UpdateComponentSpaceTransforms()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FTransform& LocalTransform = CurrentLocalSpacePose[BoneIndex];
        const int32 ParentIndex = Skeleton.Bones[BoneIndex].ParentIndex;

        if (ParentIndex == -1) // 루트 본
        {
            CurrentComponentSpacePose[BoneIndex] = LocalTransform;
        }
        else // 자식 본
        {
            const FTransform& ParentComponentTransform = CurrentComponentSpacePose[ParentIndex];
            CurrentComponentSpacePose[BoneIndex] = ParentComponentTransform.GetWorldTransform(LocalTransform);
        }
    }
}

void USkeletalMeshComponent::UpdateFinalSkinningMatrices()
{
    const FSkeleton& Skeleton = SkeletalMesh->GetSkeletalMeshData()->Skeleton;
    const int32 NumBones = Skeleton.Bones.Num();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FMatrix& InvBindPose = Skeleton.Bones[BoneIndex].InverseBindPose;
        const FMatrix ComponentPoseMatrix = CurrentComponentSpacePose[BoneIndex].ToMatrix();
        
        TempFinalSkinningMatrices[BoneIndex] = InvBindPose * ComponentPoseMatrix;
        TempFinalSkinningNormalMatrices[BoneIndex] = TempFinalSkinningMatrices[BoneIndex].Inverse().Transpose();
    }
}

void USkeletalMeshComponent::ClearAnimScriptInstance()
{
	if (AnimScriptInstance)
	{
		// Clean up the existing animation instance
		DeleteObject(AnimScriptInstance);
		AnimScriptInstance = nullptr;
	}
}

bool USkeletalMeshComponent::InitializeAnimScriptInstance()
{
	if (AnimScriptInstance == nullptr)
	{
		switch (AnimationMode)
		{
		case EAnimationMode::AnimationBlueprint:
			break;
		case EAnimationMode::AnimationSingleNode:
			AnimScriptInstance = NewObject<UAnimSingleNodeInstance>();
			break;
		case EAnimationMode::AnimationCustomMode:
			break;
		default:
			assert(false, "Unknown AnimationMode");
			break;
		}

		if (AnimScriptInstance)
		{
			//AnimScriptInstance->InitializeAnimation();
			return true;
		}
		else
		{
			return false;
		}
	}
	return true;
}

#include "pch.h"
#include "AnimSequence.h"
#include "AnimDataModel.h"
#include "SkeletalMesh.h"

IMPLEMENT_CLASS(UAnimSequence)

void UAnimSequence::GetAnimationPose(FPoseContext& OutPoseData, const FAnimExtractContext& ExtractionContext) const
{
	UAnimDataModel* DataModel = GetDataModel();
	if (!DataModel || !ExtractionContext.Skeleton)
	{
		return;
	}

	const TArray<FBoneAnimationTrack>& AnimTracks = DataModel->GetBoneAnimationTracks();
	const FSkeleton* Skeleton = ExtractionContext.Skeleton;
	const float CurrentTime = static_cast<float>(ExtractionContext.CurrentTime);
	const float PlayLength = DataModel->GetPlayLength();
	const FFrameRate& FrameRate = DataModel->GetFrameRate();
	const int32 NumberOfKeys = DataModel->GetNumberOfKeys();

	// 스켈레톤의 본 개수만큼 로컬 트랜스폼 배열 초기화
	const int32 NumBones = Skeleton->Bones.Num();
	OutPoseData.LocalTransforms.SetNum(NumBones);

	// 각 본을 T-Pose(기본 포즈)로 초기화
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		const FBone& Bone = Skeleton->Bones[BoneIndex];
		
		// 로컬 바인드 포즈 계산
		if (Bone.ParentIndex == -1)
		{
			OutPoseData.LocalTransforms[BoneIndex] = FTransform(Bone.BindPose);
		}
		else
		{
			const FMatrix& ParentInvBindPose = Skeleton->Bones[Bone.ParentIndex].InverseBindPose;
			FMatrix LocalBindMatrix = Bone.BindPose * ParentInvBindPose;
			OutPoseData.LocalTransforms[BoneIndex] = FTransform(LocalBindMatrix);
		}
	}

	// NumberOfKeys가 0이면 T-Pose 반환
	if (NumberOfKeys <= 0 || PlayLength <= 0.0f)
	{
		return;
	}

	// 현재 시간에 해당하는 프레임 인덱스 계산
	float FrameTime = CurrentTime * FrameRate.ToFloat();
	int32 FrameIndex0 = static_cast<int32>(floorf(FrameTime));
	int32 FrameIndex1 = FrameIndex0 + 1;
	float Alpha = FrameTime - static_cast<float>(FrameIndex0);

	// 루핑 처리
	if (ExtractionContext.bLooping)
	{
		FrameIndex0 = FrameIndex0 % NumberOfKeys;
		FrameIndex1 = FrameIndex1 % NumberOfKeys;
	}
	else
	{
		FrameIndex0 = std::min(FrameIndex0, NumberOfKeys - 1);
		FrameIndex1 = std::min(FrameIndex1, NumberOfKeys - 1);
	}

	// 각 애니메이션 트랙에서 포즈 추출
	for (const FBoneAnimationTrack& Track : AnimTracks)
	{
		// 스켈레톤에서 해당 본의 인덱스 찾기
		const FString& TrackNameStr = Track.Name.ToString();
		const int32* BoneIndexPtr = Skeleton->BoneNameToIndex.Find(TrackNameStr);
		if (BoneIndexPtr == nullptr)
		{
			continue; // 스켈레톤에 해당 본이 없음
		}
		const int32 BoneIndex = *BoneIndexPtr;

		const FRawAnimSequenceTrack& RawTrack = Track.InternalTrack;

		// Position 보간
		FVector Position = FVector::Zero();
		if (RawTrack.PosKeys.Num() > 0)
		{
			if (RawTrack.PosKeys.Num() == 1)
			{
				Position = RawTrack.PosKeys[0];
			}
			else
			{
				int32 PosIndex0 = std::min(FrameIndex0, RawTrack.PosKeys.Num() - 1);
				int32 PosIndex1 = std::min(FrameIndex1, RawTrack.PosKeys.Num() - 1);
				Position = FVector::Lerp(RawTrack.PosKeys[PosIndex0], RawTrack.PosKeys[PosIndex1], Alpha);
			}
		}

		// Rotation 보간 (Quaternion Slerp)
		FQuat Rotation = FQuat(0, 0, 0, 1);
		if (RawTrack.RotKeys.Num() > 0)
		{
			if (RawTrack.RotKeys.Num() == 1)
			{
				Rotation = RawTrack.RotKeys[0];
			}
			else
			{
				int32 RotIndex0 = std::min(FrameIndex0, RawTrack.RotKeys.Num() - 1);
				int32 RotIndex1 = std::min(FrameIndex1, RawTrack.RotKeys.Num() - 1);
				Rotation = FQuat::Slerp(RawTrack.RotKeys[RotIndex0], RawTrack.RotKeys[RotIndex1], Alpha);
			}
		}

		// Scale 보간
		FVector Scale = FVector::One();
		if (RawTrack.ScaleKeys.Num() > 0)
		{
			if (RawTrack.ScaleKeys.Num() == 1)
			{
				Scale = RawTrack.ScaleKeys[0];
			}
			else
			{
				int32 ScaleIndex0 = std::min(FrameIndex0, RawTrack.ScaleKeys.Num() - 1);
				int32 ScaleIndex1 = std::min(FrameIndex1, RawTrack.ScaleKeys.Num() - 1);
				Scale = FVector::Lerp(RawTrack.ScaleKeys[ScaleIndex0], RawTrack.ScaleKeys[ScaleIndex1], Alpha);
			}
		}

		// 최종 로컬 트랜스폼 설정
		OutPoseData.LocalTransforms[BoneIndex].Translation = Position;
		OutPoseData.LocalTransforms[BoneIndex].Rotation = Rotation;
		OutPoseData.LocalTransforms[BoneIndex].Scale3D = Scale;
	}
}

#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"

IMPLEMENT_CLASS(UAnimInstance)

void UAnimInstance::Initialize(USkeletalMeshComponent* InOwningComponent)
{
	OwningComponent = InOwningComponent;
	if (OwningComponent && OwningComponent->GetSkeletalMesh())
	{
		Skeleton = OwningComponent->GetSkeletalMesh()->GetSkeletonMutable();
	}
	else
	{
		Skeleton = nullptr;
	}
}

void UAnimInstance::EvaluateAnimationPose(float DeltaSeconds, FPoseContext& OutFinalBonePose)
{
	// 각 상속 클래스에서 구현한 애니메이션 업데이트 로직 호출. 이 안에서 FinalPose를 수정할 수도 있음
	NativeUpdateAnimation(DeltaSeconds);

	// 추가적인 공통 로직 들어갈 가능성 있음

	OutFinalBonePose = FinalPose;
}

AActor* UAnimInstance::GetOwningActor() const
{
	if (OwningComponent)
	{
		return OwningComponent->GetOwner();
	}
	return nullptr;
}

UWorld* UAnimInstance::GetWorld() const
{
	if (OwningComponent)
	{
		return OwningComponent->GetWorld();
	}
	return nullptr;
}

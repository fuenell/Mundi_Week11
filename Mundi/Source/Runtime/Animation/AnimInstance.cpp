#include "pch.h"
#include "AnimInstance.h"
#include "SkeletalMeshComponent.h"
#include "AnimSequence.h"

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

void UAnimInstance::CheckAnimNotifyQueue()
{
	// 큐 초기화
	NotifyQueue.Empty();

	// 현재 애니메이션 시퀀스 가져오기
	UAnimSequence* CurrentSequence = GetAnimSequence();
	if (!CurrentSequence)
	{
		return;
	}

	// 현재 시간과 루핑 정보 가져오기
	const float CurrentTime = GetCurrentAnimTime();
	const bool bLooping = IsAnimLooping();

	// 애니메이션의 노티파이들을 시간 범위에 따라 수집
	UAnimSequenceBase* SequenceBase = Cast<UAnimSequenceBase>(CurrentSequence);
	if (SequenceBase)
	{
		SequenceBase->GetNotifiesInRange(PreviousTime, CurrentTime, bLooping, NotifyQueue);
	}
}

void UAnimInstance::TriggerAnimNotifies(float DeltaSeconds)
{
	// NotifyQueue에 있는 모든 노티파이 실행
	for (const FAnimNotifyEvent& NotifyEvent : NotifyQueue)
	{
		if (NotifyEvent.Notify)
		{
			// 노티파이 실행
			NotifyEvent.Notify->Notify(OwningComponent, Cast<UAnimSequenceBase>(GetAnimSequence()));
		}
	}

	// 실행 후 큐 비우기
	NotifyQueue.Empty();
}

void UAnimInstance::EvaluateAnimationPose(float DeltaSeconds, FPoseContext& OutFinalBonePose)
{
	// 이전 시간 저장 (다음에 CheckAnimNotifyQueue에서 사용)
	PreviousTime = GetCurrentAnimTime();

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

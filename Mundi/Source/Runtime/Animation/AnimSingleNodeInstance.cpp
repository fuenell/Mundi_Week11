#include "pch.h"
#include "AnimSingleNodeInstance.h"
#include "AnimationAsset.h"

IMPLEMENT_CLASS(UAnimSingleNodeInstance)

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping, float InPlayRate)
{
	if (NewAsset != CurrentAsset)
	{
		CurrentAsset = NewAsset;
	}
	bLooping = bIsLooping;
	PlayRate = InPlayRate;
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	if (bIsPlaying && CurrentAsset)
	{
		// 애니메이션 업데이트 로직 구현 필요
		// 예: 현재 시간 계산, 애니메이션 프레임 적용 등
	}
}

void UAnimSingleNodeInstance::SetPlaying(bool bIsPlaying)
{
	bIsPlaying = bIsPlaying;
}

void UAnimSingleNodeInstance::SetLooping(bool bIsLooping)
{
	bLooping = bIsLooping;
}

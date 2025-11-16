#include "pch.h"
#include "AnimSingleNodeInstance.h"
#include "AnimationAsset.h"

void UAnimSingleNodeInstance::SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping, float InPlayRate)
{
	if (NewAsset != CurrentAsset)
	{
		CurrentAsset = NewAsset;
	}
	bLooping = bIsLooping;
	PlayRate = InPlayRate;
}

void UAnimSingleNodeInstance::SetPlaying(bool bIsPlaying)
{
	bIsPlaying = bIsPlaying;
}

void UAnimSingleNodeInstance::SetLooping(bool bIsLooping)
{
	bLooping = bIsLooping;
}

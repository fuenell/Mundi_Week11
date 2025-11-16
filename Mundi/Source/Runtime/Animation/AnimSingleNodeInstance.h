#pragma once
#include "AnimInstance.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
public:
	DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() override = default;

	void SetPlaying(bool bIsPlaying);
	void SetLooping(bool bIsLooping);

	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping = true, float InPlayRate = 1.f);
	class UAnimationAsset* GetCurrentAnimationAsset() const { return CurrentAsset; }

protected:
	class UAnimationAsset* CurrentAsset;

	bool bIsPlaying = false;
	bool bLooping = true;
	float PlayRate = 1.f;
};

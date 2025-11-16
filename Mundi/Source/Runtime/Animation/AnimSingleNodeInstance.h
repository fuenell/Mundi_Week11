#pragma once
#include "AnimInstance.h"

class UAnimationAsset;
struct FPoseContext;

class UAnimSingleNodeInstance : public UAnimInstance
{
public:
	DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() override = default;

	void SetPlaying(bool bIsPlaying);
	void SetLooping(bool bIsLooping);

	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping = true, float InPlayRate = 1.f);
	UAnimationAsset* GetCurrentAnimationAsset() const { return CurrentAsset; }

	void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UAnimationAsset* CurrentAsset = nullptr;

	bool bIsPlaying = false;
	bool bLooping = true;
	float PlayRate = 1.f;
	float CurrentTime = 0.0f; // 추가: 현재 애니메이션 재생 시간
};

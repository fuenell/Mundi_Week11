#pragma once
#include "AnimInstance.h"

class UAnimationAsset;
class UAnimSequence;
struct FPoseContext;

// 단일 애니메이션 에셋을 재생하는 애니메이션 인스턴스
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
	DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() override = default;

	void SetPlaying(bool bIsPlaying);
	void SetLooping(bool bIsLooping);
	void SetCurrentTime(float InTime, bool bForceDirty = false);

	bool IsPlaying() const { return bIsPlaying; }
	bool IsLooping() const { return bLooping; }
	float GetCurrentTime() const { return CurrentTime; }

	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping = true, float InPlayRate = 1.f);
	UAnimationAsset* GetCurrentAnimationAsset() const { return CurrentAsset; }

	UAnimSequence* GetCurrentAnimSequence() const;

	void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UAnimationAsset* CurrentAsset = nullptr; // 재생할 애니메이션. 현재는 UAnimSequence 타입만 할당됨

	bool bIsPlaying = false;
	bool bLooping = true;
	float PlayRate = 1.f;
	float CurrentTime = -1.0f; // 현재 애니메이션 재생 시간

	bool bDirtyTime = false; // 시간 변경 플래그
};

#pragma once
#include "AnimInstance.h"
#include "UAnimSingleNodeInstance.generated.h"

class UAnimationAsset;
struct FPoseContext;

// 단일 애니메이션 에셋을 재생하는 애니메이션 인스턴스
class UAnimSingleNodeInstance : public UAnimInstance
{
public:
	GENERATED_REFLECTION_BODY()

	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() override = default;

	void SetPlaying(bool bIsPlaying);
	void SetLooping(bool bIsLooping);

	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping = true, float InPlayRate = 1.f);
	UAnimationAsset* GetCurrentAnimationAsset() const { return CurrentAsset; }

	void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UAnimationAsset* CurrentAsset = nullptr; // 재생할 애니메이션. 현재는 UAnimSequence 타입만 할당됨

	bool bIsPlaying = false;
	bool bLooping = true;
	float PlayRate = 1.f;
	float CurrentTime = 0.0f; // 추가: 현재 애니메이션 재생 시간
};

#pragma once
#include "Object.h"
#include "AnimTypes.h"

class FSkeleton;

class UAnimInstance : public UObject
{
public:
	DECLARE_CLASS(UAnimInstance, UObject)
	UAnimInstance() = default;
	virtual ~UAnimInstance() override = default;

	void TriggerAnimNotifies(float DeltaSeconds);

	// Native update override point. It is usually a good idea to simply gather data in this step and 
	// for the bulk of the work to be done in NativeThreadSafeUpdateAnimation.
	virtual void NativeUpdateAnimation(float DeltaSeconds) {}

	void EvaluateAnimationPose(float DeltaSeconds, FPoseContext& OutFinalPose);
protected:
	FPoseContext FinalPose;

	FSkeleton* Skeleton = nullptr;
};

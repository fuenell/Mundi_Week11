#pragma once
#include "Name.h"
//#include "AnimCurveTypes.h"

struct FAnimNotifyEvent
{
	float TriggerTime;
	float Duration;
	FName NotifyName;
};

struct FPoseContext
{
	TArray<FTransform> LocalTransforms;
	// TODO: 애니메이션 커브
};

struct FAnimExtractContext
{
	double CurrentTime;
	bool bLooping;

	class FSkeleton* Skeleton = nullptr;
};

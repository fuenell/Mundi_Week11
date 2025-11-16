#pragma once
#include "Name.h"
//#include "AnimCurveTypes.h"

struct FAnimNotifyEvent
{
	float TriggerTime;
	float Duration;
	FName NotifyName;
};

// Output 용
struct FPoseContext
{
	TArray<FTransform> LocalTransforms; // 어떤 애니메이션의 어떤 시점의 본 로컬 트랜스폼 배열
	// TODO: 애니메이션 커브
};

// Input 용
struct FAnimExtractContext
{
	double CurrentTime; // 추출할 애니메이션의 현재 시간 (초 단위)
	bool bLooping;

	class FSkeleton* Skeleton = nullptr; // 애니메이션을 적용할 스켈레톤
};

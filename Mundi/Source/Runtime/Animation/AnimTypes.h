#pragma once
#include "Name.h"
#include "AnimNotify.h"
#include "Archive.h"
#include <cmath>
//#include "AnimCurveTypes.h"

struct FAnimNotifyEvent
{
	float TriggerTime = 0.f;
	float Duration = 0.f;      // NotifyState용 (AnimNotifyState는 아직 미구현)
	FName NotifyName;
	UAnimNotify* Notify = nullptr; // 실제 Notify 오브젝트

	bool Serialize(FArchive& Ar)
	{
		if (!Ar.IsLoading() && !Ar.IsSaving())
		{
			return false;
		}

		Ar << TriggerTime;
		Ar << Duration;

		if (Ar.IsSaving())
		{
			const FString NotifyNameStr = NotifyName.ToString();
			Serialization::WriteString(Ar, NotifyNameStr);
		}
		else
		{
			FString NotifyNameStr;
			Serialization::ReadString(Ar, NotifyNameStr);
			NotifyName = FName(NotifyNameStr);

			// 직렬화 데이터에는 실제 노티파이 인스턴스를 저장하지 않으므로
			// 로딩 시점에는 nullptr로 초기화한 뒤 런타임에서 재지정한다.
			Notify = nullptr;
		}

		return true;
	}

	void SetTriggerTime(float NewTime, float SequenceLength)
	{
		TriggerTime = NewTime;
		NormalizeTriggerTime(SequenceLength);
	}

	float GetEndTriggerTime() const
	{
		return TriggerTime + Duration;
	}

	void NormalizeTriggerTime(float SequenceLength)
	{
		if (SequenceLength <= 0.f)
		{
			if (TriggerTime < 0.f)
			{
				TriggerTime = 0.f;
			}
			return;
		}

		// 음수거나 SequenceLength보다 TriggerTime이 큰 경우 대비해 정규화
		float Normalized = std::fmod(TriggerTime, SequenceLength);
		if (Normalized < 0.f)
		{
			Normalized += SequenceLength;
		}
		TriggerTime = Normalized;
	}
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

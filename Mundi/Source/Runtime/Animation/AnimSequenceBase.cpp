#include "pch.h"
#include "AnimSequenceBase.h"

#include "FBXLoader.h"
#include <algorithm>

IMPLEMENT_CLASS(UAnimSequenceBase)

void UAnimSequenceBase::Load(const FString& InFilePath, class ID3D11Device* InDevice)
{
	//DataModel = UFbxLoader::GetInstance().LoadAnimationFromFbx(InFilePath, 0);
	DataModel = UFbxLoader::GetInstance().LoadAnimationMixamo(InFilePath);
	if(!DataModel)
	{
		UE_LOG("UAnimSequenceBase::Load: Failed to load animation data model from FBX OR No Animation: %s", InFilePath.c_str());
	}

	SetFilePath(InFilePath);
}

int32 UAnimSequenceBase::AddNotify(const FAnimNotifyEvent& InEvent)
{
	FAnimNotifyEvent NewEvent = InEvent;
	const float SequenceLength = GetSequenceLength();
	NewEvent.NormalizeTriggerTime(SequenceLength);
	NewEvent.ClampDurationToSequence(SequenceLength);
	Notifies.Add(NewEvent);
	SortNotifies();
	return Notifies.Num() - 1;
}

void UAnimSequenceBase::RemoveNotify(int32 Index)
{
	if (!Notifies.IsValidIndex(Index))
	{
		return;
	}

	Notifies.RemoveAt(Index);
}

void UAnimSequenceBase::ClearNotifies()
{
	Notifies.clear();
}

bool UAnimSequenceBase::MoveNotify(int32 Index, float NewTime)
{
	if (!Notifies.IsValidIndex(Index))
	{
		return false;
	}

	Notifies[Index].SetTriggerTime(NewTime, GetSequenceLength());
	SortNotifies();
	return true;
}

void UAnimSequenceBase::RefreshNotifyPositions()
{
	const float SequenceLength = GetSequenceLength();
	for (FAnimNotifyEvent& Event : Notifies)
	{
		Event.NormalizeTriggerTime(SequenceLength);
		Event.ClampDurationToSequence(SequenceLength);
	}
	SortNotifies();
}

void UAnimSequenceBase::GetNotifiesInRange(float PreviousTime, float CurrentTime, bool bLooping, TArray<FAnimNotifyEvent>& OutEvents) const
{
	OutEvents.clear();

	if (Notifies.empty() || PreviousTime == CurrentTime)
	{
		return;
	}

	const float SequenceLength = GetSequenceLength();
	auto CollectRange = [&](float RangeStart, float RangeEnd)
	{
		for (const FAnimNotifyEvent& Event : Notifies)
		{
			const float TriggerTime = Event.TriggerTime;
			if (RangeStart <= TriggerTime && TriggerTime < RangeEnd)
			{
				OutEvents.Add(Event);
			}
		}
	};

	if (SequenceLength <= 0.f || !bLooping)
	{
		if (PreviousTime < CurrentTime)
		{
			CollectRange(PreviousTime, CurrentTime);
		}
		else
		{
			CollectRange(CurrentTime, PreviousTime);
		}
		return;
	}

	// 루핑: Previous -> Current로 넘어가는 동안 경계를 통과할 수 있음
	if (PreviousTime < CurrentTime)
	{
		CollectRange(PreviousTime, CurrentTime);
	}
	else
	{
		CollectRange(PreviousTime, SequenceLength);
		CollectRange(0.f, CurrentTime);
	}
}

float UAnimSequenceBase::GetSequenceLength() const
{
	if (DataModel)
	{
		return DataModel->GetPlayLength();
	}
	return 0.f;
}

void UAnimSequenceBase::SortNotifies()
{
	std::stable_sort(Notifies.begin(), Notifies.end(), [](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
	{
		return A.TriggerTime < B.TriggerTime;
	});
}

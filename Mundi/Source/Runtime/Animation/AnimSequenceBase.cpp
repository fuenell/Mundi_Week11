#include "pch.h"
#include "AnimSequenceBase.h"

#include "FBXLoader.h"
#include <algorithm>

IMPLEMENT_CLASS(UAnimSequenceBase)

UAnimSequenceBase::UAnimSequenceBase()
{
	EnsureDefaultNotifyTrack();
}

void UAnimSequenceBase::Load(const FString& InFilePath, class ID3D11Device* InDevice)
{
	//DataModel = UFbxLoader::GetInstance().LoadAnimationFromFbx(InFilePath, 0);
	DataModel = UFbxLoader::GetInstance().LoadAnimationMixamo(InFilePath);
	if(!DataModel)
	{
		UE_LOG("UAnimSequenceBase::Load: Failed to load animation data model from FBX OR No Animation: %s", InFilePath.c_str());
	}

	SetFilePath(InFilePath);
	EnsureDefaultNotifyTrack();
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

bool UAnimSequenceBase::AddNotifyTrack()
{
	EnsureDefaultNotifyTrack();

	if (NotifyTracks.Num() >= MaxNumNotifyTracks)
	{
		return false;
	}

	const int32 NewTrackIndex = NotifyTracks.Num();
	const FString BaseName = "Track" + std::to_string(NewTrackIndex);
	FString CandidateName = BaseName;
	int32 SuffixIdx = 1;

	auto IsNameInUse = [&](const FName& InName) -> bool
	{
		for (const FName& Existing : NotifyTracks)
		{
			if (Existing == InName)
			{
				return true;
			}
		}
		return false;
	};

	FName FinalName(CandidateName);
	while (IsNameInUse(FinalName))
	{
		CandidateName = BaseName + "_" + std::to_string(SuffixIdx++);
		FinalName = FName(CandidateName);
	}

	NotifyTracks.Add(FinalName);
	return true;
}

bool UAnimSequenceBase::DeleteNotifyTrack(int32 TrackIndex)
{
	EnsureDefaultNotifyTrack();

	if (!NotifyTracks.IsValidIndex(TrackIndex))
	{
		return false;
	}

	for (const FAnimNotifyEvent& Event : Notifies)
	{
		if (static_cast<int32>(Event.TrackIndex) == TrackIndex)
		{
			return false; // 노티파이가 존재하는 트랙은 삭제 불가
		}
	}

	if (TrackIndex == 0 && NotifyTracks.Num() == 1)
	{
		return false;
	}

	NotifyTracks.RemoveAt(TrackIndex);

	for (FAnimNotifyEvent& Event : Notifies)
	{
		const int32 EventTrackIndex = static_cast<int32>(Event.TrackIndex);
		if (EventTrackIndex > TrackIndex)
		{
			Event.TrackIndex = static_cast<uint8>(EventTrackIndex - 1);
		}
	}

	return true;
}

bool UAnimSequenceBase::RenameNotifyTrack(int32 TrackIndex, const FName& NewName)
{
	EnsureDefaultNotifyTrack();

	if (!NotifyTracks.IsValidIndex(TrackIndex))
	{
		return false;
	}

	const FName& CurrentName = NotifyTracks[TrackIndex];
	if (CurrentName == NewName)
	{
		return true;
	}

	for (int32 Index = 0; Index < NotifyTracks.Num(); ++Index)
	{
		if (Index == TrackIndex)
		{
			continue;
		}

		if (NotifyTracks[Index] == NewName)
		{
			return false;
		}
	}

	NotifyTracks[TrackIndex] = NewName;
	return true;
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

void UAnimSequenceBase::EnsureDefaultNotifyTrack()
{
	if (NotifyTracks.IsEmpty())
	{
		NotifyTracks.Add(FName("Track0"));
	}
}

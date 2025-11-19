#pragma once
#include "Name.h"
#include "AnimNotify.h"
#include "Archive.h"
#include "ObjectFactory.h"
#include "JsonSerializer.h"
#include <exception>
#include <cmath>
//#include "AnimCurveTypes.h"

struct FAnimNotifyEvent
{
	float TriggerTime = 0.f;
	float Duration = 0.f;      // NotifyState용 (AnimNotifyState는 아직 미구현)
	FName NotifyName;
	uint8 TrackIndex = 0; // 노티파이가 속한 트랙 인덱스
	UAnimNotify* Notify = nullptr; // 실제 Notify 오브젝트
	inline static bool bSerializeNotifyProperties = false;

	static void SetSerializeNotifyProperties(bool bEnable)
	{
		bSerializeNotifyProperties = bEnable;
	}

	static bool ShouldSerializeNotifyProperties()
	{
		return bSerializeNotifyProperties;
	}

	bool Serialize(FArchive& Ar)
	{
		if (!Ar.IsLoading() && !Ar.IsSaving())
		{
			return false;
		}

		const bool bSerializeNotifyProps = ShouldSerializeNotifyProperties();

		Ar << TriggerTime;
		Ar << Duration;
		Ar << TrackIndex;

		if (Ar.IsSaving())
		{
			// NotifyName 저장
			const FString NotifyNameStr = NotifyName.ToString();
			Serialization::WriteString(Ar, NotifyNameStr);

			// Notify 객체 저장
			bool bHasNotify = (Notify != nullptr);
			Ar << bHasNotify;

			if (bHasNotify)
			{
				// Notify 클래스 이름 저장
				const FString ClassName = Notify->GetClass()->Name;
				Serialization::WriteString(Ar, ClassName);

				if (bSerializeNotifyProps)
				{
					// Notify 객체의 리플렉션 프로퍼티를 JSON으로 직렬화 후 문자열 저장
					JSON NotifyJson = JSON::Make(JSON::Class::Object);
					Notify->Serialize(false, NotifyJson);
					FString NotifyJsonString = FJsonSerializer::FormatJsonString(NotifyJson, 0);
					Serialization::WriteString(Ar, NotifyJsonString);
				}
			}
		}
		else // Loading
		{
			// NotifyName 로드
			FString NotifyNameStr;
			Serialization::ReadString(Ar, NotifyNameStr);
			NotifyName = FName(NotifyNameStr);

			// Notify 객체 로드
			bool bHasNotify = false;
			Ar << bHasNotify;

			if (bHasNotify)
			{
				// Notify 클래스 이름 로드
				FString ClassName;
				Serialization::ReadString(Ar, ClassName);

				// Notify 프로퍼티 JSON 문자열 로드 (지원되는 경우에만)
				FString NotifyJsonString;
				if (bSerializeNotifyProps)
				{
					Serialization::ReadString(Ar, NotifyJsonString);
				}

				// 클래스 찾기
				UClass* NotifyClass = UClass::FindClass(FName(ClassName));
				if (NotifyClass)
				{
					// 새 인스턴스 생성
					Notify = Cast<UAnimNotify>(ObjectFactory::NewObject(NotifyClass));
					if (Notify)
					{
						if (bSerializeNotifyProps && !NotifyJsonString.empty())
						{
							try
							{
								JSON NotifyJson = JSON::Load(NotifyJsonString);
								Notify->Serialize(true, NotifyJson);
							}
							catch (const std::exception&)
							{
								UE_LOG("FAnimNotifyEvent::Serialize: Failed to parse notify JSON for class %s", ClassName.c_str());
							}
						}
					}
					else
					{
						UE_LOG("FAnimNotifyEvent::Serialize: Failed to instantiate notify of class %s", ClassName.c_str());
					}
				}
				else
				{
					// 클래스를 찾지 못한 경우 경고
					UE_LOG("FAnimNotifyEvent::Serialize: Failed to find class: %s", ClassName.c_str());
					Notify = nullptr;
				}
			}
			else
			{
				Notify = nullptr;
			}
		}

		return true;
	}

	void SetTriggerTime(float NewTime, float SequenceLength)
	{
		TriggerTime = NewTime;
		NormalizeTriggerTime(SequenceLength);
		ClampDurationToSequence(SequenceLength);
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

	void ClampDurationToSequence(float SequenceLength)
	{
		if (Duration < 0.f)
		{
			Duration = 0.f;
		}

		if (SequenceLength <= 0.f)
		{
			return;
		}

		const float MaxDuration = SequenceLength - TriggerTime;
		if (MaxDuration <= 0.f)
		{
			Duration = 0.f;
			return;
		}

		if (Duration > MaxDuration)
		{
			Duration = MaxDuration;
		}
	}
};

// Output 용
struct FPoseContext
{
	TArray<FTransform> LocalTransforms; // 어떤 애니메이션의 어떤 시점의 본 로컬 트랜스폼 배열
	// TODO: 애니메이션 커브

	static FPoseContext Lerp(const FPoseContext& A, const FPoseContext& B, float Progress)
	{
		FPoseContext Result;
		// Todo: 길이가 다른 경우 남은 트랜스폼을 복사 or 아이덴티티로 채우기 or 블렌드 실패 처리?
		int32 Count = std::min(A.LocalTransforms.Num(), B.LocalTransforms.Num());
		Result.LocalTransforms.SetNum(Count);

		for (int32 Index = 0; Index < Count; ++Index)
		{
			Result.LocalTransforms[Index] = FTransform::Lerp(A.LocalTransforms[Index], B.LocalTransforms[Index], Progress);
		}

		return Result;
	}
};

// Input 용
struct FAnimExtractContext
{
	double CurrentTime; // 추출할 애니메이션의 현재 시간 (초 단위)
	bool bLooping;

	class FSkeleton* Skeleton = nullptr; // 애니메이션을 적용할 스켈레톤
};

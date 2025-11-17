#include "pch.h"
#include "AnimDataModel.h"
#include "Archive.h"

IMPLEMENT_CLASS(UAnimDataModel)

// ============================================================================
// FFrameRate
// ============================================================================
float FFrameRate::ToFloat() const
{
	return static_cast<float>(Numerator) / static_cast<float>(Denominator);
}

// ============================================================================
// FRawAnimSequenceTrack 직렬화
// ============================================================================
FArchive& operator<<(FArchive& Ar, FRawAnimSequenceTrack& Track)
{
	if (Ar.IsSaving())
	{
		Serialization::WriteArray(Ar, Track.PosKeys);
		Serialization::WriteArray(Ar, Track.RotKeys);
		Serialization::WriteArray(Ar, Track.ScaleKeys);
	}
	else // Loading
	{
		Serialization::ReadArray(Ar, Track.PosKeys);
		Serialization::ReadArray(Ar, Track.RotKeys);
		Serialization::ReadArray(Ar, Track.ScaleKeys);
	}
	return Ar;
}

// ============================================================================
// FBoneAnimationTrack 직렬화
// ============================================================================
FArchive& operator<<(FArchive& Ar, FBoneAnimationTrack& Track)
{
	// FName 직렬화 (ToString()으로 FString 변환 후 저장)
	if (Ar.IsSaving())
	{
		FString NameStr = Track.Name.ToString();
		Serialization::WriteString(Ar, NameStr);
	}
	else // Loading
	{
		FString NameStr;
		Serialization::ReadString(Ar, NameStr);
		Track.Name = FName(NameStr);
	}

	// InternalTrack 직렬화
	Ar << Track.InternalTrack;

	return Ar;
}

// ============================================================================
// FAnimationCurveData 직렬화
// ============================================================================
FArchive& operator<<(FArchive& Ar, FAnimationCurveData& Data)
{
	if (Ar.IsSaving())
	{
		uint32 FloatCurveCount = static_cast<uint32>(Data.FloatCurves.size());
		Ar << FloatCurveCount;
		for (auto& Curve : Data.FloatCurves)
		{
			Ar << Curve;
		}

		uint32 TransformCurveCount = static_cast<uint32>(Data.TransformCurves.size());
		Ar << TransformCurveCount;
		for (auto& Curve : Data.TransformCurves)
		{
			Ar << Curve;
		}
	}
	else // Loading
	{
		uint32 FloatCurveCount;
		Ar << FloatCurveCount;

		if (FloatCurveCount > Serialization::MAX_REASONABLE_ARRAY_SIZE)
		{
			throw std::runtime_error("Cache corrupt: FloatCurve count is unreasonable.");
		}

		Data.FloatCurves.resize(FloatCurveCount);
		for (uint32 i = 0; i < FloatCurveCount; ++i)
		{
			Ar << Data.FloatCurves[i];
		}

		uint32 TransformCurveCount;
		Ar << TransformCurveCount;

		if (TransformCurveCount > Serialization::MAX_REASONABLE_ARRAY_SIZE)
		{
			throw std::runtime_error("Cache corrupt: TransformCurve count is unreasonable.");
		}

		Data.TransformCurves.resize(TransformCurveCount);
		for (uint32 i = 0; i < TransformCurveCount; ++i)
		{
			Ar << Data.TransformCurves[i];
		}
	}

	return Ar;
}

// ============================================================================
// UAnimDataModel 직렬화
// ============================================================================
FArchive& operator<<(FArchive& Ar, UAnimDataModel& AnimData)
{
	if (Ar.IsSaving())
	{
		// BoneAnimationTracks 배열 저장
		uint32 TrackCount = static_cast<uint32>(AnimData.BoneAnimationTracks.size());
		Ar << TrackCount;
		for (auto& Track : AnimData.BoneAnimationTracks)
		{
			Ar << Track;
		}

		// 기본 데이터 저장
		Ar << AnimData.PlayLength;
		Ar << AnimData.FrameRate.Numerator;
		Ar << AnimData.FrameRate.Denominator;
		Ar << AnimData.NumberOfFrames;
		Ar << AnimData.NumberOfKeys;

		// CurveData 저장
		Ar << AnimData.CurveData;
	}
	else // Loading
	{
		// BoneAnimationTracks 배열 로드
		uint32 TrackCount;
		Ar << TrackCount;

		if (TrackCount > Serialization::MAX_REASONABLE_ARRAY_SIZE)
		{
			throw std::runtime_error("Cache corrupt: BoneAnimationTrack count is unreasonable.");
		}

		AnimData.BoneAnimationTracks.resize(TrackCount);
		for (uint32 i = 0; i < TrackCount; ++i)
		{
			Ar << AnimData.BoneAnimationTracks[i];
		}

		// 기본 데이터 로드
		Ar << AnimData.PlayLength;
		Ar << AnimData.FrameRate.Numerator;
		Ar << AnimData.FrameRate.Denominator;
		Ar << AnimData.NumberOfFrames;
		Ar << AnimData.NumberOfKeys;

		// CurveData 로드
		Ar << AnimData.CurveData;
	}

	return Ar;
}

// ============================================================================
// UAnimDataModel Getter 함수들
// ============================================================================
const TArray<FBoneAnimationTrack>& UAnimDataModel::GetBoneAnimationTracks() const
{
	return BoneAnimationTracks;
}

float UAnimDataModel::GetPlayLength() const
{
	return PlayLength;
}

const FFrameRate& UAnimDataModel::GetFrameRate() const
{
	return FrameRate;
}

int32 UAnimDataModel::GetNumberOfFrames() const
{
	return NumberOfFrames;
}

int32 UAnimDataModel::GetNumberOfKeys() const
{
	return NumberOfKeys;
}

const FAnimationCurveData& UAnimDataModel::GetCurveData() const
{
	return CurveData;
}

// ============================================================================
// UAnimDataModel Setter 함수들
// ============================================================================
void UAnimDataModel::SetPlayLength(float InPlayLength)
{
	PlayLength = InPlayLength;
}

void UAnimDataModel::SetFrameRate(int32 Numerator, int32 Denominator)
{
	FrameRate.Numerator = Numerator;
	FrameRate.Denominator = Denominator;
}

void UAnimDataModel::SetNumberOfFrames(int32 InNumFrames)
{
	NumberOfFrames = InNumFrames;
}

void UAnimDataModel::SetNumberOfKeys(int32 InNumKeys)
{
	NumberOfKeys = InNumKeys;
}

// ============================================================================
// 트랙 추가 함수들
// ============================================================================
void UAnimDataModel::AddBoneTrack(const FBoneAnimationTrack& InTrack)
{
	BoneAnimationTracks.Add(InTrack);
}

void UAnimDataModel::AddFloatCurve(const FFloatCurve& InCurve)
{
	CurveData.FloatCurves.Add(InCurve);
}

void UAnimDataModel::AddTransformCurve(const FTransformCurve& InCurve)
{
	CurveData.TransformCurves.Add(InCurve);
}

// ============================================================================
// 초기화 함수
// ============================================================================
void UAnimDataModel::ClearAllTracks()
{
	BoneAnimationTracks.clear();
	CurveData.FloatCurves.clear();
	CurveData.TransformCurves.clear();
	PlayLength = 0.0f;
	FrameRate = { 30, 1 };
	NumberOfFrames = 0;
	NumberOfKeys = 0;
}

//====================================================================================
// UAnimDataModel - 디버그 정보 출력
//====================================================================================
void UAnimDataModel::PrintDebugInfo() const
{
	UE_LOG("=== Animation Data Model Debug Info ===");
	UE_LOG("Play Length: %.2f seconds", PlayLength);
	UE_LOG("Frame Rate: %d/%d (%.2f fps)", FrameRate.Numerator, FrameRate.Denominator, FrameRate.ToFloat());
	UE_LOG("Number of Frames: %d", NumberOfFrames);
	UE_LOG("Number of Keys: %d", NumberOfKeys);
	UE_LOG("Bone Animation Tracks: %d", BoneAnimationTracks.Num());
	UE_LOG("Float Curves: %d", CurveData.FloatCurves.Num());
	UE_LOG("Transform Curves: %d", CurveData.TransformCurves.Num());
}

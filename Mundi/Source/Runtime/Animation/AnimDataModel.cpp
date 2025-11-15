#include "pch.h"
#include "AnimDataModel.h"

IMPLEMENT_CLASS(UAnimDataModel)

//====================================================================================
// FFrameRate 구현
//====================================================================================
float FFrameRate::ToFloat() const
{
	if (Denominator == 0) return 0.0f;
	return static_cast<float>(Numerator) / static_cast<float>(Denominator);
}

//====================================================================================
// UAnimDataModel - Getter 구현
//====================================================================================
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

//====================================================================================
// UAnimDataModel - Setter 구현
//====================================================================================
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

//====================================================================================
// UAnimDataModel - 트랙 추가 구현
//====================================================================================
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

//====================================================================================
// UAnimDataModel - 초기화 구현
//====================================================================================
void UAnimDataModel::ClearAllTracks()
{
	BoneAnimationTracks.Empty();
	CurveData.FloatCurves.Empty();
	CurveData.TransformCurves.Empty();
	PlayLength = 0.0f;
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

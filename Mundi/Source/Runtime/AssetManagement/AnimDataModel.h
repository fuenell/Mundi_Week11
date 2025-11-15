#pragma once
#include "UEContainer.h"
#include "Vector.h"
#include "Name.h"
#include "Object.h"
#include "AnimCurveTypes.h"

struct FFrameRate
{
	int32 Numerator;
	int32 Denominator;

	float ToFloat() const
	{
		if (Denominator == 0) return 0.0f;
		return static_cast<float>(Numerator) / static_cast<float>(Denominator);
	}
};

struct FRawAnimSequenceTrack
{
	TArray<FVector> PosKeys;   // 위치 키프레임
	TArray<FQuat>   RotKeys;   // 회전 키프레임 (Quaternion)
	TArray<FVector> ScaleKeys; // 스케일 키프레임
};

struct FBoneAnimationTrack
{
	FName Name;                        // Bone 이름
	FRawAnimSequenceTrack InternalTrack; // 실제 애니메이션 데이터
};

struct FAnimationCurveData
{
	TArray<FFloatCurve>	FloatCurves;
	TArray<FTransformCurve>	TransformCurves;
};

class UAnimDataModel : public UObject
{
	TArray<FBoneAnimationTrack> BoneAnimationTracks;
	float PlayLength;
	FFrameRate FrameRate;
	int32 NumberOfFrames;
	int32 NumberOfKeys;
	FAnimationCurveData CurveData;

	virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const
	{
		return BoneAnimationTracks;
	}

	//... 기타 멤버 함수 및 데이터 ...
};

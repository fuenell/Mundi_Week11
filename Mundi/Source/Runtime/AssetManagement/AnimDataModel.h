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

	float ToFloat() const;
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
public:
	DECLARE_CLASS(UAnimDataModel, UObject)

	UAnimDataModel() = default;
	virtual ~UAnimDataModel() override = default;

	// Getter 함수들
	virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const;
	float GetPlayLength() const;
	const FFrameRate& GetFrameRate() const;
	int32 GetNumberOfFrames() const;
	int32 GetNumberOfKeys() const;
	const FAnimationCurveData& GetCurveData() const;

	// Setter 함수들
	void SetPlayLength(float InPlayLength);
	void SetFrameRate(int32 Numerator, int32 Denominator);
	void SetNumberOfFrames(int32 InNumFrames);
	void SetNumberOfKeys(int32 InNumKeys);
	
	// 트랙 추가 함수들
	void AddBoneTrack(const FBoneAnimationTrack& InTrack);
	void AddFloatCurve(const FFloatCurve& InCurve);
	void AddTransformCurve(const FTransformCurve& InCurve);
	
	// 초기화 함수
	void ClearAllTracks();

	// 디버그 정보 출력
	void PrintDebugInfo() const;

private:
	TArray<FBoneAnimationTrack> BoneAnimationTracks;
	float PlayLength = 0.0f;
	FFrameRate FrameRate = { 30, 1 }; // 기본값: 30fps
	int32 NumberOfFrames = 0;
	int32 NumberOfKeys = 0;
	FAnimationCurveData CurveData;
};

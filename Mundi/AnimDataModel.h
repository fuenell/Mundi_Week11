#pragma once
#include "UEContainer.h"
#include "Vector.h"
#include "Name.h"
#include "Object.h"

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

//-----------------------------

enum ERichCurveInterpMode : int
{
	/** Use linear interpolation between values. */
	RCIM_Linear,
	/** Use a constant value. Represents stepped values. */
	RCIM_Constant,
	/** Cubic interpolation. See TangentMode for different cubic interpolation options. */
	RCIM_Cubic,
	/** No interpolation. */
	RCIM_None
};

enum ERichCurveTangentMode : int
{
	/** Automatically calculates tangents to create smooth curves between values. */
	RCTM_Auto,
	/** User specifies the tangent as a unified tangent where the two tangents are locked to each other, presenting a consistent curve before and after. */
	RCTM_User,
	/** User specifies the tangent as two separate broken tangents on each side of the key which can allow a sharp change in evaluation before or after. */
	RCTM_Break,
	/** No tangents. */
	RCTM_None,
	/** New Auto tangent that creates smoother curves than Auto. */
	RCTM_SmartAuto
};

enum ERichCurveTangentWeightMode : int
{
	/** Don't take tangent weights into account. */
	RCTWM_WeightedNone,
	/** Only take the arrival tangent weight into account for evaluation. */
	RCTWM_WeightedArrive,
	/** Only take the leaving tangent weight into account for evaluation. */
	RCTWM_WeightedLeave,
	/** Take both the arrival and leaving tangent weights into account for evaluation. */
	RCTWM_WeightedBoth
};

struct FRichCurveKey
{
	/** Interpolation mode between this key and the next */
	ERichCurveInterpMode InterpMode;

	/** Mode for tangents at this key */
	ERichCurveTangentMode TangentMode;

	/** If either tangent at this key is 'weighted' */
	ERichCurveTangentWeightMode TangentWeightMode;

	float Time;
	float Value;

	float ArriveTangent;
	float ArriveTangentWeight;

	float LeaveTangent;
	float LeaveTangentWeight;
};

// 원래 UE에서는 각 Curve가 FAnimCurveBase라는 것을 상속받지만, 여기서는 단순화를 위해 생략

// 원래 UE에서는 FFloatCurve가 FRichCurve를 has a로 wrapping 하지만, 여기서는 단순화를 위해 FRichCurve가 갖고 있는 데이터들을 직접 포함
struct FFloatCurve
{
	FName Name;
	TArray<FRichCurveKey> Keys;
};

struct FVectorCurve
{
	enum class EIndex
	{
		X = 0,
		Y,
		Z,
		Max
	};

	FFloatCurve	FloatCurves[3];
};

struct FTransformCurve
{
	/** Curve data for each transform. */
	FVectorCurve	TranslationCurve;

	/** Rotation curve - right now we use euler because quat also doesn't provide linear interpolation - curve editor can't handle quat interpolation
	 * If you hit gimbal lock, you should add extra key to fix it. This will cause gimbal lock.
	 * @TODO: Eventually we'll need FRotationCurve that would contain rotation curve - that will interpolate as slerp or as quaternion
	 */
	FVectorCurve	RotationCurve;
	FVectorCurve	ScaleCurve;
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

	virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const;

	//... 기타 멤버 함수 및 데이터 ...
};

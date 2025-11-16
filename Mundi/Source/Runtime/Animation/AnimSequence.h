#pragma once
#include "Object.h"
#include "AnimSequenceBase.h"

// 가장 단순한 형태의 애니메이션 에셋
class UAnimSequence : public UAnimSequenceBase
{
public:
	DECLARE_CLASS(UAnimSequence, UAnimSequenceBase)
	UAnimSequence() = default;
	virtual ~UAnimSequence() override = default;

	virtual void GetAnimationPose(FPoseContext& OutPoseData, const FAnimExtractContext& ExtractionContext) const override;
};

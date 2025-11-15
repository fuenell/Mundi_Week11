#pragma once
#include "Object.h"
#include "AnimSequenceBase.h"

class UAnimSequence : public UAnimSequenceBase
{
public:
	DECLARE_CLASS(UAnimSequence, UAnimSequenceBase)
	UAnimSequence() = default;
	virtual ~UAnimSequence() override = default;
};

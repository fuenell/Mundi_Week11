#pragma once
#include "Object.h"

class UAnimInstance : public UObject
{
public:
	DECLARE_CLASS(UAnimInstance, UObject)
	UAnimInstance() = default;
	virtual ~UAnimInstance() override = default;
};

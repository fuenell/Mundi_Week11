#pragma once
#include "Object.h"

struct FSkeleton;

class UAnimationAsset : public UObject
{
public:
	DECLARE_CLASS(UAnimationAsset, UObject)

	UAnimationAsset() = default;
	virtual ~UAnimationAsset() override = default;

private:
	FSkeleton* Skeleton = nullptr;
};

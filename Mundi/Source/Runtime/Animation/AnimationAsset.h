#pragma once
#include "Object.h"

struct FSkeleton;

class UAnimationAsset : public UResourceBase
{
public:
	DECLARE_CLASS(UAnimationAsset, UResourceBase)

	UAnimationAsset() = default;
	virtual ~UAnimationAsset() override = default;

private:
	FSkeleton* Skeleton = nullptr;
};

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
	FSkeleton* Skeleton = nullptr; // TODO: 지금 이걸 전혀 안 쓰고 있음. 뭔가 수정 필요
};

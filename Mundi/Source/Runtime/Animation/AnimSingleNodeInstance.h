#pragma once
#include "AnimInstance.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
public:
	DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)
	UAnimSingleNodeInstance() = default;
	virtual ~UAnimSingleNodeInstance() override = default;

	//void SetAnimationAsset(class UAnimationAsset* NewAsset);
	class UAnimationAsset* GetCurrentAnimationAsset() const { return CurrentAsset; }
private:
	class UAnimationAsset* CurrentAsset;
};

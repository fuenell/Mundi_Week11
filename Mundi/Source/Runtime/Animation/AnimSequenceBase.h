#pragma once
#include "AnimationAsset.h"
#include "AnimDataModel.h"
#include "AnimTypes.h"

class UAnimSequenceBase : public UAnimationAsset
{
public:
	DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)

	UAnimSequenceBase() = default;
	virtual ~UAnimSequenceBase() override = default;

	UAnimDataModel* GetDataModel() const
	{
		return DataModel;
	}

public:
	TArray<struct FAnimNotifyEvent> Notifies;

private:
	UAnimDataModel* DataModel = nullptr;

};

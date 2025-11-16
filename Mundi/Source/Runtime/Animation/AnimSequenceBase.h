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

	// ResouceManager에서 호출됨
	void Load(const FString& InFilePath, class ID3D11Device* InDevice);

	UAnimDataModel* GetDataModel() const
	{
		return DataModel;
	}

public:
	TArray<struct FAnimNotifyEvent> Notifies;

private:
	UAnimDataModel* DataModel = nullptr;

};

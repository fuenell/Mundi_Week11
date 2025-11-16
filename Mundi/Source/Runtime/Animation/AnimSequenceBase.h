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

	// DataModel을 가져오는 함수: ResouceManager에서 호출됨
	void Load(const FString& InFilePath, class ID3D11Device* InDevice);

	// ExtractionContext의 정보를 바탕으로, OutPoseData 추출
	// TODO: 순수 가상함수로 구현하려 했으나, 순수 가상함수는 IMPLEMENT_CLASS(UAnimSequenceBase)을 할 수가 없어서 보류
	virtual void GetAnimationPose(FPoseContext& OutPoseData, const FAnimExtractContext& ExtractionContext) const {} 

	UAnimDataModel* GetDataModel() const
	{
		return DataModel;
	}

public:
	TArray<struct FAnimNotifyEvent> Notifies;

private:
	UAnimDataModel* DataModel = nullptr;

};

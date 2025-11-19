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

	// ===================================================================
	// 노티파이 관리 API
	// ===================================================================

	/**@brief: AnimNotifyEvent 객체의 TriggerTime을 정규화한 후 Notifies에 추가하고 정렬. */
	int32 AddNotify(const FAnimNotifyEvent& InEvent);

	/**@brief: Index에 해당하는 노티파이의 TriggerTime을 NewTime으로 변경하고 정렬. */
	bool MoveNotify(int32 Index, float NewTime);

	/**@brief: Notifies에 있는 모든 AnimNotifyEvent의 TriggerTime 정규화 후 정렬. */
	void RefreshNotifyPositions();

	/**@brief: Index에 해당하는 노티파이를 제거. */
	void RemoveNotify(int32 Index);

	/**@brief: 노티파이 전체 삭제*/
	void ClearNotifies();

	/**@brief: Notifies 배열 전체를 const 참조로 반환. */
	const TArray<FAnimNotifyEvent>& GetNotifies() const { return Notifies; }

	/**
	 * @brief: 특정 시간 구간에 속하는 노티파이들을 OutEvents에 추가.
	 * 애니메이션 인스턴스 쪽에서 시간 계산 후 이 메소드를 통해 현재 틱에서 실행해야 할 노티파이를 얻어오는 식으로 사용.
	 */
	void GetNotifiesInRange(float PreviousTime, float CurrentTime, bool bLooping, TArray<FAnimNotifyEvent>& OutEvents) const;

protected:
	float GetSequenceLength() const;
	void SortNotifies();

public:
	TArray<struct FAnimNotifyEvent> Notifies;

private:
	UAnimDataModel* DataModel = nullptr;

};

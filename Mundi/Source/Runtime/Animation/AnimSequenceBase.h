#pragma once
#include "AnimationAsset.h"
#include "AnimDataModel.h"
#include "AnimTypes.h"
#include "AnimNotify.h"

class UAnimSequenceBase : public UAnimationAsset
{
public:
	DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)

	UAnimSequenceBase();
	virtual ~UAnimSequenceBase() override;

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

	/**
	 * @brief: 새 노티파이 트랙을 추가.
	 * 이미 트랙이 최대 개수에 도달한 경우 추가하지 않으며 false 반환. 추가 성공시 true 반환.
	 * 트랙 이름의 초기값은 "TrackX" (X는 0부터 시작하는 인덱스) 형태.
	 */
	bool AddNotifyTrack();

	/**
	 * @brief: 특정 인덱스의 노티파이 트랙을 삭제.
	 * 해당 트랙에 속한 모든 노티파이들도 함께 삭제됨.
	 * 유효하지 않은 인덱스가 주어졌거나 노티파이가 있는 트랙을 삭제하려는 경우 삭제하지 않으며 false 반환. 삭제 성공시 true 반환.
	 * 맨 끝 인덱스가 아닌 트랙을 삭제할 경우 그 뒤의 트랙들이 앞으로 한 칸씩 당겨지며, 해당 트랙에 속한 노티파이들의 TrackIndex도 함께 갱신됨.
	 */
	bool DeleteNotifyTrack(int32 TrackIndex);

	/**
	 * @brief: 특정 인덱스의 노티파이 트랙 이름을 변경.
	 * 유효하지 않은 인덱스가 주어졌거나 중복된 이름이 주어진 경우 변경하지 않으며 false 반환. 변경 성공시 true 반환.
	 */
	bool RenameNotifyTrack(int32 TrackIndex, const FName& NewName);

	float GetRateScale() const { return RateScale; }
	float GetSequenceLength() const;

protected:
	
	void SortNotifies();
	void ReleaseNotifyEvent(struct FAnimNotifyEvent& Event);

public:
	TArray<struct FAnimNotifyEvent> Notifies;
	TArray<FName> NotifyTracks;
	static constexpr int32 MaxNumNotifyTracks = 4;

private:
	UAnimDataModel* DataModel = nullptr;
	void EnsureDefaultNotifyTrack();

	/** Number for tweaking playback rate of this animation globally. */
	float RateScale = 1.0f;

};

#pragma once
#include "SkinnedMeshComponent.h"
#include "USkeletalMeshComponent.generated.h"

// TODO: UE에 있는 것 일단 그대로 가져옴. 추후 이 엔진에 맞춰 수정 가능
enum class EAnimationMode : int
{
	AnimationSingleNode, // 단일 애니메이션 에셋 재생: 보통 UAnimSingleNodeInstancex타입의 에셋 사용
	// This is custom type, engine leaves AnimInstance as it is
	AnimationCustomMode, // 가장 저수준으로 애니메이션을 조작하는 모드 (미구현)
};

class UAnimationAsset;

UCLASS(DisplayName="스켈레탈 메시 컴포넌트", Description="스켈레탈 메시를 렌더링하는 컴포넌트입니다")
class USkeletalMeshComponent : public USkinnedMeshComponent
{
public:
    GENERATED_REFLECTION_BODY()
    REGISTER_PRIMITIVE_COMPONENT(USkeletalMeshComponent)
    
    USkeletalMeshComponent();
    ~USkeletalMeshComponent() override = default;

	void BeginPlay() override;
    void TickComponent(float DeltaTime) override;
    void SetSkeletalMesh(const FString& PathFileName) override;

// Editor Section
public:
    /**
     * @brief 특정 뼈의 부모 기준 로컬 트랜스폼을 설정
     * @param BoneIndex 수정할 뼈의 인덱스
     * @param NewLocalTransform 새로운 부모 기준 로컬 FTransform
     */
    void SetBoneLocalTransform(int32 BoneIndex, const FTransform& NewLocalTransform);

    void SetBoneWorldTransform(int32 BoneIndex, const FTransform& NewWorldTransform);

	void ResetBoneTransformsToBindPose();

    /**
     * @brief 특정 뼈의 현재 로컬 트랜스폼을 반환
     */
    FTransform GetBoneLocalTransform(int32 BoneIndex) const;
    
    /**
     * @brief 기즈모를 렌더링하기 위해 특정 뼈의 월드 트랜스폼을 계산
     */
    FTransform GetBoneWorldTransform(int32 BoneIndex);

	/**
	 * @brief CurrentLocalSpacePose의 변경사항을 ComponentSpace -> FinalMatrices 계산까지 모두 수행
	 */
	void ForceRecomputePose();

protected:

	/**
	 * @brief CurrentLocalSpacePose를 기반으로 CurrentComponentSpacePose 채우기
	 */
	void UpdateComponentSpaceTransforms();

	/**
	 * @brief CurrentComponentSpacePose를 기반으로 TempFinalSkinningMatrices 채우기
	 */
	void UpdateFinalSkinningMatrices();
    
// Animation Section
public:
	void SetEnableAnimation(bool bInEnableAnimation);
	bool IsAnimationEnabled() const { return bEnableAnimation; }

	class UAnimSingleNodeInstance* GetSingleNodeInstance() const;

	void SetAnimationMode(EAnimationMode InAnimationMode, bool bForceInitAnimScriptInstance = true);

	// 단일 애니메이션 재생 관련 함수들
	void SetAnimation(UAnimationAsset* NewAnimToPlay);
	void Play(bool bLooping);
	void Stop();

	/**
	 * @brief 단일 애니메이션 재생 시작
	 * @param NewAnimToPlay 재생할 애니메이션 에셋
	 * @param bLooping 애니메이션 반복 재생 여부
	 */
	void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);

	/**
	 * @brief 현재 AnimScriptInstance가 이미 가지고 있는 단일 애니메이션을 재생
	 */
	void PlayDefaultAnimation();

protected:
	/**
	 * @brief 현재 애니메이션 인스턴스를 정리
	 */
	void ClearAnimScriptInstance();

	/**
	 * @brief AnimScriptInstance을 여러 조건(ex: AnimationMode)에 따라 초기화
	 */
	bool InitializeAnimScriptInstance();

// Editor Section
protected:

	/**
	 * @brief 각 뼈의 부모 기준 초기 로컬 트랜스폼
	 */
	TArray<FTransform> BindLocalSpacePose;

    /**
     * @brief 각 뼈의 부모 기준 로컬 트랜스폼
     */
    TArray<FTransform> CurrentLocalSpacePose;

    /**
     * @brief LocalSpacePose로부터 계산된 컴포넌트(모델 좌표계) 기준 트랜스폼
     */
    TArray<FTransform> CurrentComponentSpacePose;

    /**
     * @brief 부모에게 보낼 최종 스키닝 행렬 (임시 계산용 버퍼)
     */
    TArray<FMatrix> TempFinalSkinningMatrices;
    TArray<FMatrix> TempFinalSkinningNormalMatrices;

// Animation Section
protected:
	bool bEnableAnimation = true;

	UPROPERTY(EditAnywhere, Category = "Skeletal Mesh", Tooltip = "애니메이션 재생 모드")
	EAnimationMode AnimationMode = EAnimationMode::AnimationSingleNode;

	// 이 컴포넌트에 연결된 애니메이션 인스턴스
	UPROPERTY(EditAnywhere, Category = "Skeletal Mesh", Tooltip = "AnimScriptInstance")
	class UAnimInstance* AnimScriptInstance;

// FOR TEST!!!
private:
    float TestTime = 0;
    bool bIsInitialized = false;
    FTransform TestBoneBasePose;

	bool bPlayingAnimation = false;
};

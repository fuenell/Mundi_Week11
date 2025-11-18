#pragma once
#include "Object.h"
#include "AnimTypes.h"
#include "UAnimInstance.generated.h"

class FSkeleton;
class USkeletalMeshComponent;

// 애니메이션을 실행하는 인스턴스. 보통 이것을 상속받아 사용
class UAnimInstance : public UObject
{
public:
	GENERATED_REFLECTION_BODY()

	UAnimInstance() = default;
	virtual ~UAnimInstance() override = default;

	virtual void Initialize(USkeletalMeshComponent* InOwningComponent);

	void TriggerAnimNotifies(float DeltaSeconds);

	// 각 상속 클래스마다의 특수한 애니메이션 업데이트 로직
	// EvaluateAnimationPose 내부에서 호출됨
	virtual void NativeUpdateAnimation(float DeltaSeconds) {}

	// DeltaSeconds를 바탕으로, 애니메이션을 적용해, OutFinalPose에 최종 포즈를 할당
	void EvaluateAnimationPose(float DeltaSeconds, FPoseContext& OutFinalPose);

	virtual void SetFloat(const FString& Name, float Value) {}
	virtual void SetBool(const FString& Name, bool Value) {}
	virtual void SetInt(const FString& Name, int Value) {}

	virtual float GetFloat(const FString& Name) { return 0.0f; }
	virtual bool GetBool(const FString& Name) { return false; }
	virtual int GetInt(const FString& Name) { return 0; }

	USkeletalMeshComponent* GetOwningComponent() const { return OwningComponent; }
	AActor* GetOwningActor() const;
	UWorld* GetWorld() const;

protected:
	FPoseContext FinalPose; // 최종 포즈 데이터

	/*
		TODO: 이 변수의 각 Bone의 Name과, 애니메이션 에셋이 가진 DataModel의 Track의 Name과 매칭시켜야 함.
		하지만, 현재 로직에서는 이 둘을 따로 따로 할당하기 때문에, 매칭을 잘못하는 실수를 범할 수 있음.
		방법 강구 필요.
	*/
	// 애니메이션을 적용할 스켈레톤
	FSkeleton* Skeleton = nullptr;

private:
	USkeletalMeshComponent* OwningComponent = nullptr;
};

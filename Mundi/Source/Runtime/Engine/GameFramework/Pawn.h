#pragma once
#include "Actor.h"
#include "APawn.generated.h"

class UMovementComponent;

UCLASS(DisplayName = "Pawn", Description = "플레이어나 AI 컨트롤러에 의해 제어될 수 있는 액터입니다")
class APawn : public AActor
{
public:
	GENERATED_REFLECTION_BODY()

	APawn();
	virtual ~APawn() override;

	// Life Cycle
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay() override;

	// ───── Input System ─────────────────────────
	// 입력 처리를 위한 가상 함수들 (파생 클래스에서 오버라이드)
	virtual void SetupPlayerInput() {}  // 입력 바인딩 설정
	virtual void MoveForward(float Value);
	virtual void MoveRight(float Value);
	virtual void Turn(float Value);
	virtual void LookUp(float Value);

	// ───── Movement ─────────────────────────
	// 이동 컴포넌트 접근
	UMovementComponent* GetMovementComponent() const { return MovementComponent; }
	void SetMovementComponent(UMovementComponent* NewMovementComponent);

	// 이동 속도 제어
	void AddMovementInput(const FVector& WorldDirection, float ScaleValue = 1.0f);
	FVector GetPendingMovementInputVector() const { return PendingMovementInput; }
	FVector ConsumeMovementInputVector();

	// ───── Controller ─────────────────────────
	// 컨트롤러 (현재는 플레이어 입력만 지원, 추후 AI Controller 추가 가능)
	bool IsPlayerControlled() const { return bIsPlayerControlled; }
	void SetPlayerControlled(bool bInPlayerControlled) { bIsPlayerControlled = bInPlayerControlled; }

	// ───── Camera & View ─────────────────────────
	// 카메라 회전 제어
	virtual FQuat GetViewRotation() const;
	virtual void SetViewRotation(const FQuat& NewRotation);

	// 기본 카메라 설정 (3인칭용)
	void SetupDefaultCamera(float SpringArmLength = 300.0f, const FVector& CameraOffset = FVector(0, 0, 60));

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;

protected:
	// 이동 컴포넌트 (UMovementComponent 또는 파생 클래스)
	UMovementComponent* MovementComponent;

	// 입력 누적 벡터 (매 프레임 소비됨)
	FVector PendingMovementInput;

	// 컨트롤 상태
	UPROPERTY(EditAnywhere, Category="Pawn")
	bool bIsPlayerControlled;

	// 뷰 회전 (컨트롤러의 회전과 독립적)
	FQuat ViewRotation;

	// 회전 속도
	UPROPERTY(EditAnywhere, Category="Pawn|Camera", Range="0.1, 10.0")
	float TurnRate;

	UPROPERTY(EditAnywhere, Category="Pawn|Camera", Range="0.1, 10.0")
	float LookUpRate;
};

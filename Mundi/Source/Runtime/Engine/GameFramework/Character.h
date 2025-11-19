#pragma once
#include "Pawn.h"
#include "ACharacter.generated.h"

class USkeletalMeshComponent;
class UCapsuleComponent;
class UCameraComponent;

UCLASS(DisplayName = "Character", Description = "인간형 캐릭터의 움직임이 내장된 Pawn입니다.")
class ACharacter : public APawn
{
public:
	GENERATED_REFLECTION_BODY()

	ACharacter();
	virtual ~ACharacter() override;

	// Life Cycle
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// AActor
	FAABB GetBounds() const override;

	// ───── Mesh ─────────────────────────
	// 스켈레탈 메시 접근
	USkeletalMeshComponent* GetMesh() const { return MeshComponent; }
	void SetMesh(USkeletalMeshComponent* NewMesh);

	// ───── Collision ─────────────────────────
	// 캡슐 충돌체 접근 (인간형 캐릭터 전용)
	UCapsuleComponent* GetCapsuleComponent() const { return CollisionComponent; }

	// ───── Camera ─────────────────────────
	// 카메라 컴포넌트 접근
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	// 카메라 회전 제어 (마우스 입력)
	void AddCameraYaw(float Value);
	void AddCameraPitch(float Value);

	// ───── Movement ─────────────────────────
	// 캐릭터 전용 이동 함수들
	virtual void Jump();
	virtual void StopJumping();
	bool CanJump() const;

	// 지면 체크
	bool IsGrounded() const { return bIsGrounded; }
	void SetGrounded(bool bInGrounded) { bIsGrounded = bInGrounded; }

	// 중력 및 점프 설정
	float GetGravityScale() const { return GravityScale; }
	void SetGravityScale(float NewScale) { GravityScale = NewScale; }

	float GetJumpVelocity() const { return JumpVelocity; }
	void SetJumpVelocity(float NewVelocity) { JumpVelocity = NewVelocity; }

	// 이동 속도
	float GetWalkSpeed() const { return WalkSpeed; }
	void SetWalkSpeed(float NewSpeed) { WalkSpeed = NewSpeed; }

	float GetRunSpeed() const { return RunSpeed; }
	void SetRunSpeed(float NewSpeed) { RunSpeed = NewSpeed; }

	// 달리기 상태
	void StartSprinting() { bIsSprinting = true; }
	void StopSprinting() { bIsSprinting = false; }
	bool IsSprinting() const { return bIsSprinting; }

	// 현재 이동 속도 가져오기
	float GetCurrentSpeed() const;

	// ───── Rotation ─────────────────────────
	// 회전 관련 설정
	bool ShouldRotateToMovement() const { return bOrientRotationToMovement; }
	void SetRotateToMovement(bool bEnable) { bOrientRotationToMovement = bEnable; }

	// ───── Input ─────────────────────────
	virtual void SetupPlayerInput() override;
	virtual void MoveForward(float Value) override;
	virtual void MoveRight(float Value) override;

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	// 내부 업데이트 함수들
	void UpdateMovement(float DeltaSeconds);
	void ApplyGravity(float DeltaSeconds);
	void UpdateRotation(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds);

	// 충돌 체크
	void CheckGroundStatus();

protected:
	// ───── Character-Specific Components ─────────────────────────
	
	// 캡슐 충돌체 (인간형 캐릭터의 충돌 처리)
	UCapsuleComponent* CollisionComponent = nullptr;

	// 스켈레탈 메시 컴포넌트
	USkeletalMeshComponent* MeshComponent = nullptr;

	// 3인칭 카메라 컴포넌트
	UCameraComponent* CameraComponent = nullptr;

	// ───── Camera Properties ─────────────────────────
	
	// 카메라 오프셋 (캐릭터 기준)
	UPROPERTY(EditAnywhere, Category="Character|Camera")
	FVector CameraOffset;

	// 카메라 거리 (캐릭터로부터의 거리)
	UPROPERTY(EditAnywhere, Category="Character|Camera", Range="0.0, 1000.0")
	float CameraDistance;

	// 카메라 회전 속도
	UPROPERTY(EditAnywhere, Category="Character|Camera", Range="0.1, 10.0")
	float CameraTurnRate;

	UPROPERTY(EditAnywhere, Category="Character|Camera", Range="0.1, 10.0")
	float CameraLookUpRate;

	// 카메라 회전 각도 (독립적으로 관리)
	float CameraYaw;
	float CameraPitch;

	// ───── Movement Properties ─────────────────────────
	
	// 이동 속도
	UPROPERTY(EditAnywhere, Category="Character|Movement", Range="0.0, 1000.0")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, Category="Character|Movement", Range="0.0, 2000.0")
	float RunSpeed;

	// 중력
	UPROPERTY(EditAnywhere, Category="Character|Movement", Range="-100.0, 100.0")
	float GravityScale;

	// 점프
	UPROPERTY(EditAnywhere, Category="Character|Movement", Range="0.0, 2000.0")
	float JumpVelocity;

	UPROPERTY(EditAnywhere, Category="Character|Movement")
	int32 MaxJumpCount;

	// 회전
	UPROPERTY(EditAnywhere, Category="Character|Movement")
	bool bOrientRotationToMovement;

	UPROPERTY(EditAnywhere, Category="Character|Movement", Range="0.0, 1000.0")
	float RotationRate;

	// ───── Movement State ─────────────────────────
	FVector Velocity;
	bool bIsGrounded;
	bool bIsSprinting;
	bool bIsJumping;
	int32 CurrentJumpCount;
	
	// 지면 체크용
	float GroundCheckDistance;
	float LastGroundZ;
};

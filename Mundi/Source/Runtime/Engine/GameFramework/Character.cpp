#include "pch.h"
#include "Character.h"
#include "SkeletalMeshComponent.h"
#include "CapsuleComponent.h"
#include "MovementComponent.h"
#include "InputManager.h"
#include "World.h"

ACharacter::ACharacter()
	: MeshComponent(nullptr)
	, CollisionComponent(nullptr)
	, WalkSpeed(300.0f)
	, RunSpeed(600.0f)
	, GravityScale(9.8f)
	, JumpVelocity(500.0f)
	, MaxJumpCount(1)
	, bOrientRotationToMovement(true)
	, RotationRate(540.0f)
	, Velocity(FVector::Zero())
	, bIsGrounded(true)
	, bIsSprinting(false)
	, bIsJumping(false)
	, CurrentJumpCount(0)
	, GroundCheckDistance(5.0f)
	, LastGroundZ(0.0f)
{
	// 캡슐 충돌체 생성 (인간형 캐릭터의 특징)
	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	SetRootComponent(CollisionComponent);

	// 스켈레탈 메시 컴포넌트 생성
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	
	// 메시를 캡슐에 부착
	if (MeshComponent && CollisionComponent)
	{
		MeshComponent->SetupAttachment(CollisionComponent, EAttachmentRule::KeepRelative);
		
		// 메시 위치 조정 (캡슐 중심에서 약간 아래로)
		MeshComponent->SetRelativeLocation(FVector(0, 0, -90.0f));
		MeshComponent->SetRelativeRotation(FQuat::MakeFromEulerZYX(FVector(0, -90, 0)));
	}

	// 부모의 멤버인 이동 컴포넌트 생성
	UMovementComponent* TempMovementComponent = CreateDefaultSubobject<UMovementComponent>("MovementComponent");
	SetMovementComponent(TempMovementComponent);
}

ACharacter::~ACharacter()
{
}

void ACharacter::BeginPlay()
{
	Super::BeginPlay();

	// 초기 지면 위치 저장
	LastGroundZ = GetActorLocation().Z;
}

void ACharacter::Tick(float DeltaSeconds)
{
	// APawn의 입력 처리 먼저 수행
	Super::Tick(DeltaSeconds);

	// 플레이 중일 때만 캐릭터 물리 업데이트
	if (GetWorld() && GetWorld()->bPie)
	{
		// 지면 상태 체크
		CheckGroundStatus();

		// 중력 적용
		ApplyGravity(DeltaSeconds);

		// 이동 업데이트
		UpdateMovement(DeltaSeconds);

		// 회전 업데이트
		UpdateRotation(DeltaSeconds);

		// 추가 입력 처리 (점프 등)
		if (bIsPlayerControlled)
		{
			UInputManager& InputManager = UInputManager::GetInstance();
			
			// 점프 입력
			if (InputManager.IsKeyPressed(VK_SPACE))
			{
				Jump();
			}
			else if (InputManager.IsKeyReleased(VK_SPACE))
			{
				StopJumping();
			}

			// 달리기 입력
			if (InputManager.IsKeyDown(VK_SHIFT))
			{
				StartSprinting();
			}
			else
			{
				StopSprinting();
			}
		}
	}
}

void ACharacter::SetupPlayerInput()
{
	Super::SetupPlayerInput();
	// 추가 입력 바인딩이 필요하면 여기에 구현
}

void ACharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// 카메라 방향 기준 전방 이동 (수평면)
		FVector Forward = GetActorForward();
		Forward.Z = 0.0f;
		Forward = Forward.GetSafeNormal();
		
		float Speed = bIsSprinting ? RunSpeed : WalkSpeed;
		AddMovementInput(Forward, Value * Speed);
	}
}

void ACharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// 카메라 방향 기준 오른쪽 이동 (수평면)
		FVector Right = GetActorRight();
		Right.Z = 0.0f;
		Right = Right.GetSafeNormal();
		
		float Speed = bIsSprinting ? RunSpeed : WalkSpeed;
		AddMovementInput(Right, Value * Speed);
	}
}

void ACharacter::Jump()
{
	if (CanJump())
	{
		bIsJumping = true;
		Velocity.Z = JumpVelocity;
		CurrentJumpCount++;
	}
}

void ACharacter::StopJumping()
{
	bIsJumping = false;
}

bool ACharacter::CanJump() const
{
	// 지면에 있거나 다중 점프 횟수가 남아있으면 점프 가능
	return (bIsGrounded || CurrentJumpCount < MaxJumpCount);
}

void ACharacter::CheckGroundStatus()
{
	// 간단한 지면 체크 (실제로는 Raycast나 충돌 체크 사용)
	FVector Location = GetActorLocation();
	
	// Z 속도가 0 이하이고 위치가 지면 근처이면 착지
	if (Velocity.Z <= 0.0f && FMath::Abs(Location.Z - LastGroundZ) < GroundCheckDistance)
	{
		if (!bIsGrounded)
		{
			// 착지
			bIsGrounded = true;
			CurrentJumpCount = 0;
			Velocity.Z = 0.0f;
			
			// 정확한 지면 위치로 보정
			Location.Z = LastGroundZ;
			SetActorLocation(Location);
		}
	}
	else if (Location.Z > LastGroundZ + GroundCheckDistance)
	{
		// 공중에 있음
		bIsGrounded = false;
	}
}

void ACharacter::ApplyGravity(float DeltaSeconds)
{
	if (!bIsGrounded)
	{
		// 중력 가속도 적용
		Velocity.Z -= GravityScale * 100.0f * DeltaSeconds;
		
		// 최대 낙하 속도 제한 (선택적)
		const float MaxFallSpeed = -2000.0f;
		if (Velocity.Z < MaxFallSpeed)
		{
			Velocity.Z = MaxFallSpeed;
		}
	}
}

void ACharacter::UpdateMovement(float DeltaSeconds)
{
	// 수평 이동 입력 처리
	FVector MovementInput = ConsumeMovementInputVector();
	if (!MovementInput.IsZero())
	{
		// 수평 속도 설정
		Velocity.X = MovementInput.X;
		Velocity.Y = MovementInput.Y;
	}
	else
	{
		// 감속 (마찰)
		const float Deceleration = 10.0f;
		Velocity.X = FMath::Lerp(Velocity.X, 0.0f, Deceleration * DeltaSeconds);
		Velocity.Y = FMath::Lerp(Velocity.Y, 0.0f, Deceleration * DeltaSeconds);
	}

	// 위치 업데이트
	if (!Velocity.IsZero())
	{
		FVector DeltaLocation = Velocity * DeltaSeconds;
		AddActorWorldLocation(DeltaLocation);
	}
}

void ACharacter::UpdateRotation(float DeltaSeconds)
{
	if (bOrientRotationToMovement && !Velocity.IsZero())
	{
		// 이동 방향으로 회전
		FVector HorizontalVelocity = Velocity;
		HorizontalVelocity.Z = 0.0f;
		
		if (HorizontalVelocity.SizeSquared() > 1.0f)
		{
			FVector Direction = HorizontalVelocity.GetSafeNormal();
			FQuat TargetRotation = FQuat::FromAxisAngle(FVector(0, 0, 1), 
				std::atan2(Direction.Y, Direction.X));
			
			// 부드러운 회전 보간
			FQuat CurrentRotation = GetActorRotation();
			float InterpSpeed = RotationRate * DeltaSeconds;
			FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation, InterpSpeed);
			
			SetActorRotation(NewRotation);
		}
	}
}

void ACharacter::SetMesh(USkeletalMeshComponent* NewMesh)
{
	MeshComponent = NewMesh;
	if (MeshComponent && CollisionComponent)
	{
		MeshComponent->SetupAttachment(CollisionComponent, EAttachmentRule::KeepRelative);
	}
}

float ACharacter::GetCurrentSpeed() const
{
	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.0f;
	return HorizontalVelocity.Size();
}

void ACharacter::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// CollisionComponent와 MeshComponent는 OwnedComponents에서 처리됨
	// 추가 복사 로직이 필요하면 여기에 구현
}


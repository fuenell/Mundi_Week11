#include "pch.h"
#include "Pawn.h"
#include "MovementComponent.h"
#include "InputManager.h"

APawn::APawn()
	: MovementComponent(nullptr)
	, PendingMovementInput(FVector::Zero())
	, bIsPlayerControlled(false)
	, ViewRotation(FQuat::Identity())
	, TurnRate(2.0f)
	, LookUpRate(2.0f)
{
	// APawn은 형태에 대한 가정이 없으므로 기본 충돌체를 생성하지 않습니다.
	// 필요한 경우 파생 클래스에서 구체적인 충돌체를 생성합니다.
}

APawn::~APawn()
{
}

void APawn::BeginPlay()
{
	Super::BeginPlay();

	// 뷰 회전 초기화 (액터 회전과 동기화)
	ViewRotation = GetActorRotation();

	// 입력 설정
	if (bIsPlayerControlled)
	{
		SetupPlayerInput();
	}
}

void APawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 플레이어 제어 중일 때 입력 처리
	if (bIsPlayerControlled && GetWorld() && GetWorld()->bPie)
	{
		UInputManager& InputManager = UInputManager::GetInstance();

		// 이동 입력
		if (InputManager.IsKeyDown('W')) MoveForward(1.0f);
		if (InputManager.IsKeyDown('S')) MoveForward(-1.0f);
		if (InputManager.IsKeyDown('A')) MoveRight(-1.0f);
		if (InputManager.IsKeyDown('D')) MoveRight(1.0f);

		// 마우스 회전
		FVector2D MouseDelta = InputManager.GetMouseDelta();
		if (MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f)
		{
			Turn(MouseDelta.X * 0.05f);  // 마우스 감도 조절
			LookUp(-MouseDelta.Y * 0.05f);
		}
	}

	// 누적된 입력을 이동 컴포넌트에 전달
	if (MovementComponent && !PendingMovementInput.IsZero())
	{
		FVector InputVector = ConsumeMovementInputVector();
		MovementComponent->SetVelocity(InputVector);
	}
}

void APawn::EndPlay()
{
	Super::EndPlay();
}

void APawn::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// 전방 벡터를 기준으로 이동
		FVector Forward = GetActorForward();
		Forward.Z = 0.0f;  // 수평 이동만
		Forward = Forward.GetSafeNormal();
		AddMovementInput(Forward, Value);
	}
}

void APawn::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// 오른쪽 벡터를 기준으로 이동
		FVector Right = GetActorRight();
		Right.Z = 0.0f;  // 수평 이동만
		Right = Right.GetSafeNormal();
		AddMovementInput(Right, Value);
	}
}

void APawn::Turn(float Value)
{
	if (Value != 0.0f)
	{
		// Yaw 회전 (좌우)
		float DeltaYaw = Value * TurnRate;
		FQuat DeltaRotation = FQuat::FromAxisAngle(FVector(0, 0, 1), DegreesToRadians(DeltaYaw));
		ViewRotation = DeltaRotation * ViewRotation;
		SetActorRotation(ViewRotation);
	}
}

void APawn::LookUp(float Value)
{
	if (Value != 0.0f)
	{
		// Pitch 회전 (상하)
		float DeltaPitch = Value * LookUpRate;
		FQuat DeltaRotation = FQuat::FromAxisAngle(GetActorRight(), DegreesToRadians(DeltaPitch));
		ViewRotation = DeltaRotation * ViewRotation;
		SetActorRotation(ViewRotation);
	}
}

void APawn::SetMovementComponent(UMovementComponent* NewMovementComponent)
{
	MovementComponent = NewMovementComponent;
	if (MovementComponent)
	{
		MovementComponent->SetUpdatedComponent(RootComponent);
	}
}

void APawn::AddMovementInput(const FVector& WorldDirection, float ScaleValue)
{
	PendingMovementInput += WorldDirection * ScaleValue;
}

FVector APawn::ConsumeMovementInputVector()
{
	FVector Result = PendingMovementInput;
	PendingMovementInput = FVector::Zero();
	return Result;
}

FQuat APawn::GetViewRotation() const
{
	return ViewRotation;
}

void APawn::SetViewRotation(const FQuat& NewRotation)
{
	ViewRotation = NewRotation;
}

void APawn::SetupDefaultCamera(float SpringArmLength, const FVector& CameraOffset)
{
	// 추후 SpringArm과 Camera 컴포넌트 추가 시 구현
	// 현재는 기본 구조만 제공
}

void APawn::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	// MovementComponent는 OwnedComponents에서 처리됨
	// 추가 복사 로직이 필요하면 여기에 구현
}



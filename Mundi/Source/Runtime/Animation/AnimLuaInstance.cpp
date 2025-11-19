#include "pch.h"
#include "AnimLuaInstance.h"
#include "AnimationAsset.h"
#include "AnimSequence.h"
#include "LuaManager.h"
#include "SkeletalMeshComponent.h"

IMPLEMENT_CLASS(UAnimLuaInstance)

UAnimLuaInstance::~UAnimLuaInstance()
{
	CleanupLuaResources();
}

void UAnimLuaInstance::Initialize(USkeletalMeshComponent* InOwningComponent)
{
	Super::Initialize(InOwningComponent);

	if (ScriptFilePath.empty() == false)
	{
		LoadScript(ScriptFilePath);
	}
}

void UAnimLuaInstance::LoadScript(const FString& Path)
{
	ScriptFilePath = Path;

	CleanupLuaResources();	// 스크립트 교체 시 이전 Lua 객체 삭제 및 정리

	if (ScriptFilePath.empty() || !GetWorld())
	{
		return;
	}

	auto LuaVM = GetWorld()->GetLuaManager();
	Lua = &(LuaVM->GetState());

	// 독립된 환경 생성, Engine Object&Util 주입
	Env = LuaVM->CreateEnvironment();

	Env["StartCoroutine"] = [LuaVM, this](sol::function f)
		{
			sol::state_view L = LuaVM->GetState();

			sol::thread Thread = sol::thread::create(L); // Coroutine 관리할 Thread 생성
			sol::state_view ThreadState = Thread.state();

			sol::coroutine Coroutine(ThreadState.lua_state(), f);                // 스레드에 함수 올리기
			return LuaVM->GetScheduler().Register(std::move(Thread), std::move(Coroutine), this);
		};

	// NOTE: 리플랙션 Lua 바인딩 쓰려다가, 애니메이션 관리 스크립트이기 때문에 전용 함수가 필요해서 수동 바인딩

	// 애니메이션 재생 함수 바인딩
	// Lua에서 호출: PlayAnimationByName("Run", true, 1.0, false)
	Env.set_function("PlayAnimationByName", [this](const FString& Key, bool bLoop, float Rate, bool bForceReset)
		{
			this->PlayAnimationByName(Key, bLoop, Rate, bForceReset);
		});

	// Lua에서 호출: StopAnimation()
	Env.set_function("StopAnimation", [this]()
		{
			SetPlaying(false);
		});

	// 변수 제어 함수 바인딩
	// Lua에서 호출: SetFloat("Speed", 2.5)
	Env.set_function("SetFloat", [this](const FString& Key, float Value)
		{
			this->SetFloat(Key, Value);
		});

	// Lua에서 호출: GetFloat("Speed")
	Env.set_function("GetFloat", [this](const FString& Key) -> float
		{
			return this->GetFloat(Key);
		});

	// Lua에서 호출: SetBool("IsMoving", true)
	Env.set_function("SetBool", [this](const FString& Key, bool Value)
		{
			this->SetBool(Key, Value);
		});

	// Lua에서 호출: GetBool("IsMoving")
	Env.set_function("GetBool", [this](const FString& Key) -> bool
		{
			return this->GetBool(Key);
		});

	// Lua에서 호출: SetInt("AttackType", 3)
	Env.set_function("SetInt", [this](const FString& Key, int Value)
		{
			this->SetInt(Key, Value);
		});

	// Lua에서 호출: GetInt("AttackType")
	Env.set_function("GetInt", [this](const FString& Key) -> int
		{
			return this->GetInt(Key);
		});

	// InputManger 주입
	(*Lua)["InputManager"] = &UInputManager::GetInstance();

	if (!LuaVM->LoadScriptInto(Env, ScriptFilePath))
	{
		UE_LOG("[Lua][error] failed to run: %s\n", ScriptFilePath.c_str());
#ifdef _EDITOR
		GEngine.EndPIE();
#endif
		return;
	}

	// 함수 캐시
	FuncInitialize = FLuaManager::GetFunc(Env, "Initialize");
	FuncUpdateAnimation = FLuaManager::GetFunc(Env, "UpdateAnimation");

	if (FuncInitialize.valid())
	{
		auto Result = FuncInitialize();
		if (!Result.valid())
		{
			sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what());
#ifdef _EDITOR
			GEngine.EndPIE();
#endif
		}
	}

	bIsLuaCleanedUp = false;
}

void UAnimLuaInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// ==================[테스트용 속도 넣는 코드]==================
	if (GetOwningComponent())
	{
		static FVector PreWorldLocation;

		FVector WorldLocation = GetOwningComponent()->GetWorldLocation();

		float Speed = std::abs((WorldLocation - PreWorldLocation).Size());
		SetFloat("Speed", Speed);
		PreWorldLocation = WorldLocation;
	}
	// =======================================================

	if (FuncUpdateAnimation.valid())
	{
		auto Result = FuncUpdateAnimation(DeltaSeconds);
		if (!Result.valid()) { sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what()); }
	}

	if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentAsset))
	{
		// 2. 애니메이션 길이 가져오기
		UAnimDataModel* DataModel = AnimSequence->GetDataModel();
		if (!DataModel)
		{
			return;
		}

		if (bIsPlaying)
		{
			// 1. 현재 시간 업데이트
			CurrentTime += DeltaSeconds * PlayRate;

			float AnimLength = DataModel->GetPlayLength();

			// 3. 루핑 또는 클램핑 처리
			if (bLooping)
			{
				// 루핑: 애니메이션 길이를 넘으면 처음으로 되돌림
				if (AnimLength > 0.0f)
				{
					CurrentTime = fmodf(CurrentTime, AnimLength);
					if (CurrentTime < 0.0f)
					{
						CurrentTime += AnimLength;
					}
				}
			}
			else
			{
				// 논루핑: 애니메이션 길이를 넘으면 마지막 프레임에 고정
				if (CurrentTime >= AnimLength)
				{
					CurrentTime = AnimLength;
					bIsPlaying = false; // 애니메이션 종료
				}
			}
		}

		// 4. 애니메이션 포즈 추출을 위한 컨텍스트 설정
		FAnimExtractContext ExtractContext;
		ExtractContext.CurrentTime = CurrentTime;
		ExtractContext.bLooping = bLooping;
		ExtractContext.Skeleton = Skeleton;

		// 5. 애니메이션 포즈 추출
		FPoseContext OutPoseContext;
		AnimSequence->GetAnimationPose(OutPoseContext, ExtractContext);

		// 6. 최종 포즈 저장
		FinalPose = OutPoseContext;
	}
}

void UAnimLuaInstance::SetPlaying(bool bInIsPlaying)
{
	bIsPlaying = bInIsPlaying;
}

void UAnimLuaInstance::SetLooping(bool bIsLooping)
{
	bLooping = bIsLooping;
}

void UAnimLuaInstance::PlayAnimationByName(const FString& PathFileName, bool bLoop, float Rate, bool bForceReset)
{
	UAnimSequence* NewAnimAsset = UResourceManager::GetInstance().Load<UAnimSequence>(PathFileName);
	if (!NewAnimAsset) return;

	// 강제 리셋이 꺼져있고(false), 이미 같은 애니메이션이 재생 중이라면 -> 무시
	if (!bForceReset && CurrentAsset == NewAnimAsset && bIsPlaying)
	{
		// 속도나 루프 설정은 바뀔 수 있으니 갱신
		if (PlayRate != Rate) PlayRate = Rate;
		if (bLooping != bLoop) bLooping = bLoop;
		return;
	}

	// 여기까지 왔으면 (다른 애니메이션이거나 OR 강제 리셋이거나)
	CurrentAsset = NewAnimAsset;
	bLooping = bLoop;
	PlayRate = Rate;

	CurrentTime = 0.0f; // 0초부터 다시 시작
	bIsPlaying = true;
}

void UAnimLuaInstance::CleanupLuaResources()
{
	// 이미 정리되었다면 중복 실행 방지
	if (bIsLuaCleanedUp)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (FLuaManager* LuaVM = World->GetLuaManager())
		{
			// 코루틴 정리 (가장 중요. Use-After-Free 방지)
			LuaVM->GetScheduler().CancelByOwner(this);
		}
	}

	// Lua 참조 해제
	FuncInitialize = sol::nil;
	FuncUpdateAnimation = sol::nil;
	Env = sol::nil;
	Lua = nullptr;

	bIsLuaCleanedUp = true;
}

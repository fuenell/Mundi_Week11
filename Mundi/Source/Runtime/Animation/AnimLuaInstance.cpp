#include "pch.h"
#include "AnimLuaInstance.h"
#include "AnimationAsset.h"
#include "AnimSequence.h"
#include "LuaManager.h"

IMPLEMENT_CLASS(UAnimLuaInstance)

UAnimLuaInstance::~UAnimLuaInstance()
{
	CleanupLuaResources();
}

void UAnimLuaInstance::Initialize(USkeletalMeshComponent* InOwningComponent)
{
	Super::Initialize(InOwningComponent);

	ScriptFilePath = "Data/Scripts/NewScript.lua";

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

	if (!LuaVM->LoadScriptInto(Env, ScriptFilePath))
	{
		UE_LOG("[Lua][error] failed to run: %s\n", ScriptFilePath.c_str());
#ifdef _EDITOR
		GEngine.EndPIE();
#endif
		return;
	}

	// InputManger 주입
	(*Lua)["InputManager"] = &UInputManager::GetInstance();
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
	if (FuncUpdateAnimation.valid())
	{
		auto Result = FuncUpdateAnimation(DeltaSeconds);
		if (!Result.valid()) { sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what()); }
	}

	if (bIsPlaying && CurrentAsset)
	{
		if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(CurrentAsset))
		{
			// 1. 현재 시간 업데이트
			CurrentTime += DeltaSeconds * PlayRate;

			// 2. 애니메이션 길이 가져오기
			UAnimDataModel* DataModel = AnimSequence->GetDataModel();
			if (!DataModel)
			{
				return;
			}

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
}

void UAnimLuaInstance::SetPlaying(bool bInIsPlaying)
{
	bIsPlaying = bInIsPlaying;
}

void UAnimLuaInstance::SetLooping(bool bIsLooping)
{
	bLooping = bIsLooping;
}

void UAnimLuaInstance::CleanupLuaResources()
{
	// 이미 정리되었다면 중복 실행 방지
	if (bIsLuaCleanedUp)
	{
		return;
	}

	// GetWorld()나 LuaManager가 유효한지 확인 (소멸 시점에는 이미 없을 수 있음)
	if (UWorld* World = GetWorld())
	{
		if (FLuaManager* LuaVM = World->GetLuaManager())
		{
			// 1. 코루틴 정리 (가장 중요. Use-After-Free 방지)
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

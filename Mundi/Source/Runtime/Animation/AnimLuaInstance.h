#pragma once
#include "AnimInstance.h"
#include "LuaCoroutineScheduler.h"
#include "UAnimLuaInstance.generated.h"

class UAnimationAsset;

class UAnimLuaInstance : public UAnimInstance
{
public:
	GENERATED_REFLECTION_BODY()

	UAnimLuaInstance() = default;
	~UAnimLuaInstance() override;

	void Initialize(USkeletalMeshComponent* InOwningComponent) override;

	void LoadScript(const FString& Path);

	void SetPlaying(bool bIsPlaying);
	void SetLooping(bool bIsLooping);

	// Lua에서 호출할 함수들 (바인딩 필요)
	void PlayAnimationByName(const FString& AnimName, bool bLoop, float Rate);
	void StopAnimation();

	void NativeUpdateAnimation(float DeltaSeconds) override;

	void SetFloat(const FString& Name, float Value) override { FloatParams[Name] = Value; }
	void SetBool(const FString& Name, bool Value) override { BoolParams[Name] = Value; }
	void SetInt(const FString& Name, int Value) override { IntParams[Name] = Value; }

	float GetFloat(const FString& Name) override { return FloatParams.Contains(Name) ? FloatParams[Name] : 0.0f; }
	bool GetBool(const FString& Name) override { return BoolParams.Contains(Name) ? BoolParams[Name] : false; }
	int GetInt(const FString& Name) override { return IntParams.Contains(Name) ? IntParams[Name] : 0; }

	void CleanupLuaResources();

protected:
	UAnimationAsset* CurrentAsset = nullptr; // 재생할 애니메이션. 현재는 UAnimSequence 타입만 할당됨

	bool bIsPlaying = false;
	bool bLooping = true;
	float PlayRate = 1.f;
	float CurrentTime = 0.0f; // 추가: 현재 애니메이션 재생 시간

	TMap<FString, float> FloatParams;
	TMap<FString, bool> BoolParams;
	TMap<FString, int> IntParams;

	FString ScriptFilePath{};

	sol::state* Lua = nullptr;
	sol::environment Env{};

	sol::protected_function FuncInitialize{};
	sol::protected_function FuncUpdateAnimation{};

	bool bIsLuaCleanedUp = true;
};

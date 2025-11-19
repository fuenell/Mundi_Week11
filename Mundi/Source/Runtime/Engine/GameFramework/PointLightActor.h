#pragma once

#include "Actor.h"
#include "APointLightActor.generated.h"

class UPointLightComponent;

UCLASS(DisplayName="포인트 라이트", Description="점광원을 생성하는 액터입니다")
class APointLightActor : public AActor
{
public:

	GENERATED_REFLECTION_BODY()


	APointLightActor();
protected:
	~APointLightActor() override;

public:
	UPointLightComponent* GetLightComponent() const { return LightComponent; }

	void DuplicateSubObjects() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
	UPointLightComponent* LightComponent;
};

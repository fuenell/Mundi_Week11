#pragma once

#include "ShapeComponent.h"
#include "USphereComponent.generated.h"

UCLASS(DisplayName="구체 컴포넌트", Description="구체 모양 충돌 컴포넌트입니다")
class USphereComponent : public UShapeComponent
{
public:

    GENERATED_REFLECTION_BODY();
    REGISTER_PRIMITIVE_COMPONENT(USphereComponent)

    USphereComponent();
    void OnRegister(UWorld* InWorld) override;

    // Duplication
    virtual void DuplicateSubObjects() override;

private:
    UPROPERTY(EditAnywhere, Category="SphereRaidus")
    float SphereRadius = 0;

    void GetShape(FShape& Out) const override;

public:

    void RenderDebugVolume(class URenderer* Renderer) const override;
};

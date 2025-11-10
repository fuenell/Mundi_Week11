#pragma once
#include "Actor.h"
#include "SkinnedMeshComponent.h"
#include "ASkinnedMeshActor.generated.h"

UCLASS(DisplayName="스킨드 메시", Description="스켈레탈(스킨드) 메시를 배치하는 액터입니다")

class ASkinnedMeshActor : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    ASkinnedMeshActor();
    ~ASkinnedMeshActor() override;

    // AActor
    void Tick(float DeltaTime) override;
    FAABB GetBounds() const override;

    // Component access
    USkinnedMeshComponent* GetSkinnedMeshComponent() const { return SkinnedMeshComponent; }
    void SetSkinnedMeshComponent(USkinnedMeshComponent* InComp);

    // Convenience: forward to component
    void SetSkeletalMesh(const FString& PathFileName);

    // Copy/Serialize
    void DuplicateSubObjects() override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

protected:
    USkinnedMeshComponent* SkinnedMeshComponent = nullptr;
};


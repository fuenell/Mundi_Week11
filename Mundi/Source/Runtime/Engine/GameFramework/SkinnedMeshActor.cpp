#include "pch.h"
#include "SkinnedMeshActor.h"

ASkinnedMeshActor::ASkinnedMeshActor()
{
    ObjectName = "Skinned Mesh Actor";

    SkinnedMeshComponent = CreateDefaultSubobject<USkinnedMeshComponent>("SkinnedMeshComponent");
    // Make it root
    RootComponent = SkinnedMeshComponent;
}

ASkinnedMeshActor::~ASkinnedMeshActor() = default;

void ASkinnedMeshActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

FAABB ASkinnedMeshActor::GetBounds() const
{
    // Be robust to component replacement: query current root
    if (auto* Current = Cast<USkinnedMeshComponent>(RootComponent))
    {
        return Current->GetWorldAABB();
    }
    return FAABB();
}

void ASkinnedMeshActor::SetSkinnedMeshComponent(USkinnedMeshComponent* InComp)
{
    SkinnedMeshComponent = InComp;
}

void ASkinnedMeshActor::SetSkeletalMesh(const FString& PathFileName)
{
    if (SkinnedMeshComponent)
    {
        SkinnedMeshComponent->SetSkeletalMesh(PathFileName);
    }
}

void ASkinnedMeshActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
    for (UActorComponent* Component : OwnedComponents)
    {
        if (auto* Comp = Cast<USkinnedMeshComponent>(Component))
        {
            SkinnedMeshComponent = Comp;
            break;
        }
    }
}

void ASkinnedMeshActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        SkinnedMeshComponent = Cast<USkinnedMeshComponent>(RootComponent);
    }
}


#include "pch.h"
#include "BoneAnchorComponent.h"
#include "SelectionManager.h"

IMPLEMENT_CLASS(UBoneAnchorComponent)

void UBoneAnchorComponent::SetTarget(USkeletalMeshComponent* InTarget, int32 InBoneIndex)
{
    Target = InTarget;
    BoneIndex = InBoneIndex;
    UpdateAnchorFromBone();
}

void UBoneAnchorComponent::UpdateAnchorFromBone()
{
    if (!Target || BoneIndex < 0)
        return;

    const FTransform BoneWorld = Target->GetBoneWorldTransform(BoneIndex);
    SetWorldTransform(BoneWorld) ;
}

void UBoneAnchorComponent::OnTransformUpdated()
{
    Super::OnTransformUpdated();

    if (!Target || BoneIndex < 0)
        return;

    const FTransform AnchorWorld = GetWorldTransform();
    Target->SetBoneWorldTransform(BoneIndex, AnchorWorld);
}

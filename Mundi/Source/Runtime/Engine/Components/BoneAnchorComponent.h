#pragma once

#include "SceneComponent.h"
#include "SkeletalMeshComponent.h"

// A single anchor component that represents the transform of a selected bone.
// The viewer selects this component so the editor gizmo latches onto it.
class UBoneAnchorComponent : public USceneComponent
{
public:
    DECLARE_CLASS(UBoneAnchorComponent, USceneComponent)

    void SetTarget(USkeletalMeshComponent* InTarget, int32 InBoneIndex);
    void SetBoneIndex(int32 InBoneIndex) { BoneIndex = InBoneIndex; }
    int32 GetBoneIndex() const { return BoneIndex; }
    USkeletalMeshComponent* GetTarget() const { return Target; }

    // Updates this anchor's world transform from the target bone's current transform
    void UpdateAnchorFromBone();

    // When user moves gizmo, write back to the bone
    void OnTransformUpdated() override;

private:
    USkeletalMeshComponent* Target = nullptr;
    int32 BoneIndex = -1;
};

#pragma once
#include "Object.h"
#include "AnimNotify.h"
#include "UAnimNotify_PlaySound.generated.h"

class UAnimNotify_PlaySound : public UAnimNotify
{
public:
	GENERATED_REFLECTION_BODY()

	UAnimNotify_PlaySound() = default;
	virtual ~UAnimNotify_PlaySound() override = default;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};

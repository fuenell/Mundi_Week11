#pragma once
#include "Object.h"
#include "AnimNotify.h"
#include "UAnimNotify_PlaySound.generated.h"

class USound;

class UAnimNotify_PlaySound : public UAnimNotify
{
public:
	GENERATED_REFLECTION_BODY()

	UAnimNotify_PlaySound() = default;
	virtual ~UAnimNotify_PlaySound() override = default;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	// Getter methods
	USound* GetSound() const { return Sound; }
	float GetVolumeMultiplier() const { return VolumeMultiplier; }
	float GetPitch() const { return Pitch; }

	// Setter methods
	void SetSound(USound* InSound) { Sound = InSound; }
	void SetVolumeMultiplier(float InVolumeMultiplier) { VolumeMultiplier = InVolumeMultiplier; }
	void SetPitch(float InPitch) { Pitch = InPitch; }

protected:
	UPROPERTY(EditAnywhere, Category = "Sound")
	USound* Sound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Sound")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Audio", Tooltip = "Pitch (frequency ratio)")
	float Pitch = 1.0f;
};

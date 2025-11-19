#include "pch.h"
#include "AnimNotify_PlaySound.h"
#include "Source/Runtime/Engine/Audio/Sound.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Core/Object/Actor.h"
#include "Source/Runtime/Engine/GameFramework/FAudioDevice.h"

void UAnimNotify_PlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp || !Sound)
	{
		return;
	}

	// Get the component's world location for 3D sound positioning
	FVector SoundLocation = MeshComp->GetWorldLocation();

	UWorld* World = MeshComp->GetWorld();
	IXAudio2SourceVoice* SourceVoice = nullptr;
	if (World && World->GetWorldType() == EWorldType::PreviewMinimal)
	{
		SourceVoice = FAudioDevice::PlaySound2D(Sound, VolumeMultiplier);
	}
	else
	{
		SourceVoice = FAudioDevice::PlaySound3D(
			Sound,
			SoundLocation,
			VolumeMultiplier,
			false
		);
	}

	if (SourceVoice)
	{
		SourceVoice->SetFrequencyRatio(Pitch);

		UE_LOG("AnimNotify_PlaySound: Playing sound '%s' at location (%.1f, %.1f, %.1f) with volume %.2f",
			Sound->GetName().c_str(),
			SoundLocation.X, SoundLocation.Y, SoundLocation.Z,
			VolumeMultiplier);
	}
	else
	{
		UE_LOG("AnimNotify_PlaySound: Failed to play sound '%s'",
			Sound->GetName().c_str());
	}
}

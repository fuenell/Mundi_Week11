#pragma once


#include <xaudio2.h>
#include <x3daudio.h>
#include <Vector.h>
#pragma comment(lib, "xaudio2.lib")
class USound;
class UWorld;

// Minimal XAudio2 device bootstrap and 3D helpers
class FAudioDevice
{
public:
    static UINT32 GetOutputChannelCount();
    static bool Initialize();
    static void Shutdown();
    static void Update();

    static IXAudio2SourceVoice* PlaySound3D(USound* SoundToPlay, const FVector& EmitterPosition, float Volume = 1.0f, bool bIsLooping = false);
    static IXAudio2SourceVoice* PlaySound2D(USound* SoundToPlay, float Volume = 1.0f, bool bIsLooping = false);
    static void StopSound(IXAudio2SourceVoice* pSourceVoice);

    static void SetListenerPosition(const FVector& Position, const FVector& ForwardVec, const FVector& UpVec);
    static void UpdateSoundPosition(IXAudio2SourceVoice* pSourceVoice, const FVector& EmitterPosition);

    // Loads .wav files under GDataDir/Audio into resource manager
    static void Preload();

private:
    static IXAudio2*                pXAudio2;
    static IXAudio2MasteringVoice*  pMasteringVoice;
    static X3DAUDIO_HANDLE          X3DInstance;
    static X3DAUDIO_LISTENER        Listener;
    static DWORD                    dwChannelMask;
};


#pragma once

#if OPENGLENGINE_USE_WWISE
#include <AK/AkPlatforms.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <samples/SoundEngine/Win32/AkDefaultIOHookDeferred.h>
#if !defined(NDEBUG)
#include <AK/Comm/AkCommunication.h>
#endif
#endif

using AudioObjectId = uint64_t;

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    bool Init();
    void Update();
    void Shutdown();

    void RegisterGameObject(AudioObjectId gameObject, const char* name);
    void UnregisterGameObject(AudioObjectId gameObject);
    void SetPosition(AudioObjectId gameObject, float x, float y, float z);
    void SetListenerPosition(float x, float y, float z);

    void PlayEvent(const char* eventName, AudioObjectId gameObject);
    void StopAll();

    void SetMuted(bool value);
    bool IsMuted() const
    {
        return muted;
    }

    void SetRTPCValue(const char* rtpcName, float value);
    void SetFireVolume(float volume0to100);
    void SetAmbientVolume(float volume0to100);
    void SetMasterVolume(float volume0to100);

    static constexpr AudioObjectId AmbientId = 0xFFFFFFFF00000002ull;

private:
    static constexpr AudioObjectId ListenerId = 0xFFFFFFFF00000001ull;

#if OPENGLENGINE_USE_WWISE
    CAkDefaultIOHookDeferred g_lowLevelIO;

    bool memoryMgrInitialized = false;
    bool streamMgrCreated = false;
    bool lowLevelIoInitialized = false;
    bool soundEngineInitialized = false;
    bool commInitialized = false;
#endif
    bool initialized = false;
    bool muted = false;
};

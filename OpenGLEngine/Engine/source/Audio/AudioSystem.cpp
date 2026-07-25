#include "Audio/AudioSystem.h"

#include <cstdio>

AudioSystem::AudioSystem() {}
AudioSystem::~AudioSystem() {}

bool AudioSystem::Init()
{
#if OPENGLENGINE_USE_WWISE
    AkMemSettings memSettings;
    AK::MemoryMgr::GetDefaultSettings(memSettings);
    if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: MemoryMgr::Init failed\n");
        return false;
    }

    AkStreamMgrSettings stmSettings;
    AK::StreamMgr::GetDefaultSettings(stmSettings);
    if (!AK::StreamMgr::Create(stmSettings))
    {
        std::fprintf(stderr, "AudioSystem: StreamMgr::Create failed\n");
        return false;
    }

    AkDeviceSettings deviceSettings;
    AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
    if (g_lowLevelIO.Init(deviceSettings) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: low-level IO Init failed\n");
        return false;
    }
    g_lowLevelIO.SetBasePath(L"Assets/Wwise/");

    AkInitSettings initSettings;
    AkPlatformInitSettings platformSettings;
    AK::SoundEngine::GetDefaultInitSettings(initSettings);
    AK::SoundEngine::GetDefaultPlatformInitSettings(platformSettings);
    if (AK::SoundEngine::Init(&initSettings, &platformSettings) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: SoundEngine::Init failed\n");
        return false;
    }

#if !defined(NDEBUG)
    AkCommSettings commSettings;
    AK::Comm::GetDefaultInitSettings(commSettings);
    if (AK::Comm::Init(commSettings) != AK_Success)
        std::fprintf(stderr, "AudioSystem: Comm::Init failed (profiler connection unavailable)\n");
#endif

    RegisterGameObject(ListenerId, "Listener");
    AkGameObjectID listenerId = static_cast<AkGameObjectID>(ListenerId);
    AK::SoundEngine::SetDefaultListeners(&listenerId, 1);

    RegisterGameObject(AmbientId, "Ambient");

    AkBankID initBank = AK_INVALID_BANK_ID;
    AkBankID mainBank = AK_INVALID_BANK_ID;

    if (AK::SoundEngine::LoadBank("Init.bnk", initBank) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: failed to load Init.bnk (missing SoundBank?)\n");
        return false;
    }
    if (AK::SoundEngine::LoadBank("Main.bnk", mainBank) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: failed to load Main.bnk (missing SoundBank?)\n");
        return false;
    }

    initialized = true;
    return true;
#else
    return true;
#endif
}

void AudioSystem::Update()
{
#if OPENGLENGINE_USE_WWISE
    if (initialized)
        AK::SoundEngine::RenderAudio();
#endif
}

void AudioSystem::Shutdown()
{
#if OPENGLENGINE_USE_WWISE
    if (!initialized)
        return;

    AK::SoundEngine::UnregisterAllGameObj();

#if !defined(NDEBUG)
    AK::Comm::Term();
#endif

    AK::SoundEngine::Term();

    g_lowLevelIO.Term();
    if (AK::IAkStreamMgr::Get())
        AK::IAkStreamMgr::Get()->Destroy();
    AK::MemoryMgr::Term();

    initialized = false;
#endif
}

void AudioSystem::RegisterGameObject(AudioObjectId gameObject, const char* name)
{
#if OPENGLENGINE_USE_WWISE
    AK::SoundEngine::RegisterGameObj(static_cast<AkGameObjectID>(gameObject), name);
#else
    (void)gameObject;
    (void)name;
#endif
}

void AudioSystem::UnregisterGameObject(AudioObjectId gameObject)
{
#if OPENGLENGINE_USE_WWISE
    AK::SoundEngine::UnregisterGameObj(static_cast<AkGameObjectID>(gameObject));
#else
    (void)gameObject;
#endif
}

void AudioSystem::SetPosition(AudioObjectId gameObject, float x, float y, float z)
{
#if OPENGLENGINE_USE_WWISE
    AkSoundPosition soundPos;
    soundPos.SetPosition(x, y, z);
    soundPos.SetOrientation(0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    AK::SoundEngine::SetPosition(static_cast<AkGameObjectID>(gameObject), soundPos);
#else
    (void)gameObject;
    (void)x;
    (void)y;
    (void)z;
#endif
}

void AudioSystem::SetListenerPosition(float x, float y, float z)
{
    SetPosition(ListenerId, x, y, z);
}

void AudioSystem::PlayEvent(const char* eventName, AudioObjectId gameObject)
{
#if OPENGLENGINE_USE_WWISE
    if (!initialized)
        return;
    AK::SoundEngine::PostEvent(eventName, static_cast<AkGameObjectID>(gameObject));
#else
    (void)eventName;
    (void)gameObject;
#endif
}

void AudioSystem::SetRTPCValue(const char* rtpcName, float value)
{
#if OPENGLENGINE_USE_WWISE
    if (!initialized)
        return;
    AK::SoundEngine::SetRTPCValue(rtpcName, value);
#else
    (void)rtpcName;
    (void)value;
#endif
}

void AudioSystem::SetFireVolume(float volume0to100)
{
    SetRTPCValue("Volume_Fire", volume0to100);
}

void AudioSystem::SetAmbientVolume(float volume0to100)
{
    SetRTPCValue("Volume_Ambient", volume0to100);
}

void AudioSystem::SetMasterVolume(float volume0to100)
{
    SetRTPCValue("Volume_Master", volume0to100);
}

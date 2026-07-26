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
        Shutdown();
        return false;
    }
    memoryMgrInitialized = true;

    AkStreamMgrSettings stmSettings;
    AK::StreamMgr::GetDefaultSettings(stmSettings);
    if (!AK::StreamMgr::Create(stmSettings))
    {
        std::fprintf(stderr, "AudioSystem: StreamMgr::Create failed\n");
        Shutdown();
        return false;
    }
    streamMgrCreated = true;

    AkDeviceSettings deviceSettings;
    AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
    const AKRESULT lowLevelIoResult = g_lowLevelIO.Init(deviceSettings);
    lowLevelIoInitialized = true;
    if (lowLevelIoResult != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: low-level IO Init failed\n");
        Shutdown();
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
        Shutdown();
        return false;
    }
    soundEngineInitialized = true;

#if !defined(NDEBUG)
    AkCommSettings commSettings;
    AK::Comm::GetDefaultInitSettings(commSettings);
    if (AK::Comm::Init(commSettings) != AK_Success)
        std::fprintf(stderr, "AudioSystem: Comm::Init failed (profiler connection unavailable)\n");
    else
        commInitialized = true;
#endif

    RegisterGameObject(ListenerId, "Listener");
    AkGameObjectID listenerId = ListenerId;
    AK::SoundEngine::SetDefaultListeners(&listenerId, 1);

    RegisterGameObject(AmbientId, "Ambient");

    AkBankID initBank = AK_INVALID_BANK_ID;
    AkBankID mainBank = AK_INVALID_BANK_ID;

    if (AK::SoundEngine::LoadBank("Init.bnk", initBank) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: failed to load Init.bnk (missing SoundBank?)\n");
        Shutdown();
        return false;
    }
    if (AK::SoundEngine::LoadBank("Main.bnk", mainBank) != AK_Success)
    {
        std::fprintf(stderr, "AudioSystem: failed to load Main.bnk (missing SoundBank?)\n");
        Shutdown();
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
    if (soundEngineInitialized)
        AK::SoundEngine::UnregisterAllGameObj();

#if !defined(NDEBUG)
    if (commInitialized)
    {
        AK::Comm::Term();
        commInitialized = false;
    }
#endif

    if (soundEngineInitialized)
    {
        AK::SoundEngine::Term();
        soundEngineInitialized = false;
    }

    if (lowLevelIoInitialized)
    {
        g_lowLevelIO.Term();
        lowLevelIoInitialized = false;
    }

    if (streamMgrCreated)
    {
        if (AK::IAkStreamMgr::Get())
            AK::IAkStreamMgr::Get()->Destroy();
        streamMgrCreated = false;
    }

    if (memoryMgrInitialized)
    {
        AK::MemoryMgr::Term();
        memoryMgrInitialized = false;
    }

    initialized = false;
#endif
}

void AudioSystem::RegisterGameObject(AudioObjectId gameObject, const char* name)
{
#if OPENGLENGINE_USE_WWISE
    AK::SoundEngine::RegisterGameObj(gameObject, name);
#else
    (void)gameObject;
    (void)name;
#endif
}

void AudioSystem::UnregisterGameObject(AudioObjectId gameObject)
{
#if OPENGLENGINE_USE_WWISE
    AK::SoundEngine::UnregisterGameObj(gameObject);
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
    AK::SoundEngine::SetPosition(gameObject, soundPos);
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
    AK::SoundEngine::PostEvent(eventName, gameObject);
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

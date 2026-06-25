#include "Audio/AudioSystem.h"
AudioSystem::AudioSystem() { }
AudioSystem::~AudioSystem() { }

bool AudioSystem::Init()
{
#if OPENGLENGINE_USE_WWISE
	AkMemSettings memSettings;
	AK::MemoryMgr::GetDefaultSettings(memSettings);
	AK::MemoryMgr::Init(&memSettings);

	AkStreamMgrSettings stmSettings;
	AK::StreamMgr::GetDefaultSettings(stmSettings);
	AK::StreamMgr::Create(stmSettings);

	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
	g_lowLevelIO.Init(deviceSettings);
	g_lowLevelIO.SetBasePath(L"Assets/Wwise/");

	AkInitSettings initSettings;
	AkPlatformInitSettings platformSettings;
	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformSettings);
	AK::SoundEngine::Init(&initSettings, &platformSettings);

	AkBankID initBank = AK_INVALID_BANK_ID;
	AkBankID mainBank = AK_INVALID_BANK_ID;

	if (AK::SoundEngine::LoadBank("Init.bnk", initBank) != AK_Success)
		return false;
	if (AK::SoundEngine::LoadBank("Main.bnk", mainBank) != AK_Success)
		return false;

	return true;
#else
	return true;
#endif
}

void AudioSystem::Update()
{
#if OPENGLENGINE_USE_WWISE
	AK::SoundEngine::RenderAudio();
#endif
}

void AudioSystem::Shutdown()
{
#if OPENGLENGINE_USE_WWISE
	AK::SoundEngine::UnregisterAllGameObj();
	AK::SoundEngine::Term();

	g_lowLevelIO.Term();
	AK::IAkStreamMgr::Get()->Destroy();
	AK::MemoryMgr::Term();
#endif
}

void AudioSystem::PlayEvent(const char* eventName, uint64_t gameObject)
{
#if OPENGLENGINE_USE_WWISE
	AK::SoundEngine::RegisterGameObj(gameObject, "GameObject");
	AK::SoundEngine::PostEvent(eventName, gameObject);
#else
	(void)eventName;
	(void)gameObject;
#endif
}


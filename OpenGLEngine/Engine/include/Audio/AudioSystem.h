#pragma once
#include <cstdint>
#if OPENGLENGINE_USE_WWISE
#include <AK/AkPlatforms.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <samples/SoundEngine/Win32/AkDefaultIOHookDeferred.h>
#endif

class AudioSystem
{
	public:
		AudioSystem();
		~AudioSystem();

		bool Init();
		void Update();
		void Shutdown();

		void PlayEvent(const char* eventName, uint64_t gameObject);

	private:
#if OPENGLENGINE_USE_WWISE
		CAkDefaultIOHookDeferred g_lowLevelIO;
#endif
};


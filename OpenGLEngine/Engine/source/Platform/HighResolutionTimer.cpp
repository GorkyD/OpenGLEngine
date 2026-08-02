#include "Platform/HighResolutionTimer.h"

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>

namespace
{
constexpr unsigned int timerPeriodMs = 1;
}

HighResolutionTimer::HighResolutionTimer()
{
    timeBeginPeriod(timerPeriodMs);
}

HighResolutionTimer::~HighResolutionTimer()
{
    timeEndPeriod(timerPeriodMs);
}
#else
HighResolutionTimer::HighResolutionTimer() {}
HighResolutionTimer::~HighResolutionTimer() {}
#endif

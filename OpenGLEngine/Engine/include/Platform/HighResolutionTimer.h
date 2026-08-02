#pragma once

class HighResolutionTimer
{
public:
    HighResolutionTimer();
    ~HighResolutionTimer();

    HighResolutionTimer(const HighResolutionTimer&) = delete;
    HighResolutionTimer& operator=(const HighResolutionTimer&) = delete;
};

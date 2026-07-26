#pragma once

#include "Math/Vector3.h"

struct AutoOrbitComponent
{
    Vector3 transitionStartPos = {0, 0, 0};
    Vector3 center = {0, 0, 0};

    float radius = 8.0f;
    float height = 3.0f;
    float angularSpeed = 0.25f;

    float idleTimeout = 4.0f;
    float transitionDuration = 1.5f;

    float angle = 0.0f;
    float idleTimer = 0.0f;
    float transitionT = 0.0f;
    
    float transitionStartYaw = 0.0f;
    float transitionStartPitch = 0.0f;

    bool transitioning = false;
    bool active = true;
};

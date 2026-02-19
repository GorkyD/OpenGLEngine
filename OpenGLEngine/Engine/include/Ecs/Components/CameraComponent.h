#pragma once
#include "Math/Vector3.h"

struct CameraComponent
{
	Vector3 forward = { 0, 0, 1 };
	Vector3 right = { 1, 0, 0 };
	Vector3 up = { 0, 1, 0 };

	float yaw = 0.0f;
	float pitch = 0.0f;

   float fovY = 0.7854f;
   float nearPlane = 0.01f;
   float farPlane = 100.0f;

   bool isActive = true;
};

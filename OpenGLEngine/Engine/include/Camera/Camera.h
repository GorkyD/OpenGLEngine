#pragma once
#include "Math/OMath4.h"
#include "Math/OVector3.h"

class Camera
{
	public:
		Camera();

		OMath4 GetViewMatrix() const;

		OVector3 GetPosition() const { return position; }
		OVector3 GetForward() const { return forward; }
		OVector3 GetRight() const { return right; }
		OVector3 GetUp() const { return up; }

		float GetYaw() const { return yaw; }
		float GetPitch() const { return pitch; }

		void SetPosition(const OVector3& pos);
		void SetYaw(float yaw);
		void SetPitch(float pitch);

	private:
		void UpdateVectors();

		OVector3 position = OVector3(0, 0, 0);

		float yaw = 0.0f;
		float pitch = 0.0f;

		OVector3 forward = OVector3(0, 0, 1);
		OVector3 right = OVector3(1, 0, 0);
		OVector3 up = OVector3(0, 1, 0);
};

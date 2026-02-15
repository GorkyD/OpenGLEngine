#pragma once
#include "Math/Matrix4.h"
#include "Math/Vector3.h"

class Camera
{
	public:
		Camera();

		Matrix4 GetViewMatrix() const;

		Vector3 GetPosition() const { return position; }
		Vector3 GetForward() const { return forward; }
		Vector3 GetRight() const { return right; }
		Vector3 GetUp() const { return up; }

		float GetYaw() const { return yaw; }
		float GetPitch() const { return pitch; }

		void SetPosition(const Vector3& pos);
		void SetYaw(float yaw);
		void SetPitch(float pitch);

	private:
		void UpdateVectors();

		Vector3 position = Vector3(0, 0, 0);

		float yaw = 0.0f;
		float pitch = 0.0f;

		Vector3 forward = Vector3(0, 0, 1);
		Vector3 right = Vector3(1, 0, 0);
		Vector3 up = Vector3(0, 1, 0);
};

#include "Camera/Camera.h"
#include <cmath>

Camera::Camera()
{
	UpdateVectors();
}

void Camera::SetPosition(const Vector3& pos)
{
	position = pos;
}

void Camera::SetYaw(float y)
{
	yaw = y;
	UpdateVectors();
}

void Camera::SetPitch(float p)
{
	pitch = p;
	UpdateVectors();
}

void Camera::UpdateVectors()
{
	forward.x = std::cos(pitch) * std::sin(yaw);
	forward.y = std::sin(pitch);
	forward.z = std::cos(pitch) * std::cos(yaw);
	forward = Vector3::Normalize(forward);

	Vector3 worldUp(0, 1, 0);
	right = Vector3::Normalize(Vector3::Cross(worldUp, forward));
	up = Vector3::Cross(forward, right);
}

Matrix4 Camera::GetViewMatrix() const
{
	Matrix4 view;
	view.SetLookAtLeftHanded(position, position + forward, Vector3(0, 1, 0));
	return view;
}

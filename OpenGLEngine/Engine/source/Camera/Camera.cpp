#include "Camera/Camera.h"
#include <cmath>

Camera::Camera()
{
	UpdateVectors();
}

void Camera::SetPosition(const OVector3& pos)
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
	forward = OVector3::Normalize(forward);

	OVector3 worldUp(0, 1, 0);
	right = OVector3::Normalize(OVector3::Cross(worldUp, forward));
	up = OVector3::Cross(forward, right);
}

OMath4 Camera::GetViewMatrix() const
{
	OMath4 view;
	view.SetLookAtLeftHanded(position, position + forward, OVector3(0, 1, 0));
	return view;
}
#include "Camera/CameraMovement.h"
#include "Camera/Camera.h"
#include "Input/InputSystem.h"

static constexpr float MAX_PITCH = 1.5533f;

static float Clamp(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

void CameraMovement::Update(Camera& camera, const InputSystem& input, float deltaTime)
{
	if (input.IsLeftMouseDown())
	{
		float yaw = camera.GetYaw() + input.GetMouseDeltaX() * sensitivity;
		float pitch = camera.GetPitch() - input.GetMouseDeltaY() * sensitivity;
		pitch = Clamp(pitch, -MAX_PITCH, MAX_PITCH);
		camera.SetYaw(yaw);
		camera.SetPitch(pitch);
	}

	Vector3 forward = camera.GetForward();
	Vector3 right = camera.GetRight();
	Vector3 moveDir(0, 0, 0);

	if (input.IsKeyDown(Key::W)) moveDir += forward;
	if (input.IsKeyDown(Key::S)) moveDir -= forward;
	if (input.IsKeyDown(Key::D)) moveDir += right;
	if (input.IsKeyDown(Key::A)) moveDir -= right;
	if (input.IsKeyDown(Key::Space))  moveDir += Vector3(0, 1, 0);
	if (input.IsKeyDown(Key::LShift)) moveDir -= Vector3(0, 1, 0);

	moveDir = Vector3::Normalize(moveDir);
	camera.SetPosition(camera.GetPosition() + moveDir * (moveSpeed * deltaTime));
}

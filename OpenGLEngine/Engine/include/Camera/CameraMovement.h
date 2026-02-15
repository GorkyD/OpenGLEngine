#pragma once

class Camera;
class InputSystem;

class CameraMovement
{
	public:
		void Update(Camera& camera, const InputSystem& input, float deltaTime);

	private:
		float moveSpeed = 3.0f;
		float sensitivity = 0.003f;
};

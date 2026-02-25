#pragma once
#include "Math/Vector3.h"
#include "Math/Matrix4.h"

struct RigidbodyComponent
{
	float mass = 1.0f;
	float angularDamping = 0.995f;

	Vector3 velocity = { 0, 0, 0 };
	Vector3 force = { 0, 0, 0 };

	Vector3 angularVelocity = { 0, 0, 0 };
	Vector3 torque = { 0, 0, 0 };

	void AddForce(const Vector3& additiveForce)
	{
		force += additiveForce;
	}

	void AddTorque(const Vector3& additiveTorque)
	{
		torque += additiveTorque;
	}

	void Integrate(Vector3& position, Matrix4& rotation, const Vector3& scale, float deltaTime)
	{
		const Vector3 acceleration = force / mass;
		velocity += acceleration * deltaTime;
		position += velocity * deltaTime;
		force = { 0, 0, 0 };

		float avgDim = (scale.x + scale.y + scale.z) / 3.0f;
		float inertia = (1.0f / 6.0f) * mass * avgDim * avgDim;
		if (inertia < 1e-6f) inertia = 1e-6f;

		angularVelocity += (torque / inertia) * deltaTime;
		torque = { 0, 0, 0 };

		float angSpeed = std::sqrt(
			angularVelocity.x * angularVelocity.x +
			angularVelocity.y * angularVelocity.y +
			angularVelocity.z * angularVelocity.z);

		if (angSpeed > 1e-6f)
		{
			Vector3 axis = angularVelocity / angSpeed;
			float angle = angSpeed * deltaTime;
			Matrix4 deltaRot = Matrix4::FromAxisAngle(axis, angle);
			rotation = rotation * deltaRot;
		}

		angularVelocity = angularVelocity * angularDamping;
	}

	float GetInertia(const Vector3& scale) const
	{
		float avgDim = (scale.x + scale.y + scale.z) / 3.0f;
		float inertia = (1.0f / 6.0f) * mass * avgDim * avgDim;
		return (inertia < 1e-6f) ? 1e-6f : inertia;
	}
};

#pragma once
#include "Math/Vector3.h"

struct RigidbodyComponent
{
	float mass = 1.0f;

	Vector3 velocity = { 0,0, 0 };
	Vector3 force = { 0,0,0 };

	void AddForce(const Vector3& additiveForce)
	{
		force += additiveForce;
	}

	void Integrate(Vector3& position, float deltaTime)
	{
		const Vector3 acceleration = force / mass;
		velocity += acceleration * deltaTime;
		position += velocity * deltaTime;
		force = { 0,0,0 };
	}
};

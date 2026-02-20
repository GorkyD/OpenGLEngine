#pragma once
#include "Math/Vector3.h"
#include <algorithm>

struct AABB
{
	Vector3 min;
	Vector3 max;
};

struct CollisionInfo
{
	bool collided = false;
	Vector3 normal;
	float depth = 0.0f;
};

inline CollisionInfo CheckCollision(const AABB& a, const AABB& b)
{
	CollisionInfo info;

	float overlapX = (std::min)(a.max.x, b.max.x) - (std::max)(a.min.x, b.min.x);
	float overlapY = (std::min)(a.max.y, b.max.y) - (std::max)(a.min.y, b.min.y);
	float overlapZ = (std::min)(a.max.z, b.max.z) - (std::max)(a.min.z, b.min.z);

	if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
		return info;

	info.collided = true;

	if (overlapX <= overlapY && overlapX <= overlapZ)
	{
		info.depth = overlapX;
		info.normal = { (a.min.x + a.max.x) < (b.min.x + b.max.x) ? 1.0f : -1.0f, 0, 0 };
	}
	else if (overlapY <= overlapX && overlapY <= overlapZ)
	{
		info.depth = overlapY;
		info.normal = { 0, (a.min.y + a.max.y) < (b.min.y + b.max.y) ? 1.0f : -1.0f, 0 };
	}
	else
	{
		info.depth = overlapZ;
		info.normal = { 0, 0, (a.min.z + a.max.z) < (b.min.z + b.max.z) ? 1.0f : -1.0f };
	}

	return info;
}

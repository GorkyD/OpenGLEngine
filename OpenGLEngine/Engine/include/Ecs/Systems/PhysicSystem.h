#pragma once
#include "Ecs/Components/RigidbodyComponent.h"
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Ecs/Core/EcsWorld.h"
#include <vector>
#include <cmath>

class PhysicSystem : public IEcsSystem
{
	public:
		PhysicSystem() {}

		void Run(EcsWorld& world, float deltaTime) override
		{
			auto& rigidbodyPool = world.GetPool<RigidbodyComponent>();
			auto& transformPool = world.GetPool<TransformComponent>();
			auto& aabbPool = world.GetPool<AABB>();

			for (auto& pair : rigidbodyPool)
			{
			auto& entity = pair.first;
			auto& rigidbody = pair.second;
				rigidbody.AddForce({ 0, gravity * rigidbody.mass, 0 });

				if (transformPool.Has(entity))
				{
					auto& transform = transformPool.Get(entity);
					rigidbody.Integrate(transform.position, deltaTime);
				}
			}

			std::vector<Entity> collidables;
			for (auto& pair : aabbPool)
			{
				auto& entity = pair.first;

				if (transformPool.Has(entity))
					collidables.push_back(entity);
			}

			for (const auto entity : collidables)
			{
				auto& transform = transformPool.Get(entity);
				auto& aabb = aabbPool.Get(entity);
				Vector3 half = transform.scale * 0.5f;
				Vector3 rotatedHalf = RotateHalfExtents(half, transform.rotation);
				aabb.min = transform.position - rotatedHalf;
				aabb.max = transform.position + rotatedHalf;
			}

			for (size_t i = 0; i < collidables.size(); ++i)
			{
				for (size_t j = i + 1; j < collidables.size(); ++j)
				{
					Entity a = collidables[i];
					Entity b = collidables[j];

				CollisionInfo info = CheckCollision(aabbPool.Get(a), aabbPool.Get(b));
				if (!info.collided) continue;

				auto& tA = transformPool.Get(a);
				auto& tB = transformPool.Get(b);

				bool hasRbA = rigidbodyPool.Has(a);
				bool hasRbB = rigidbodyPool.Has(b);

				if (info.normal.y != 0.0f && !IsSupported(tA, tB, info.normal))
					continue;

				float invMassA = hasRbA ? 1.0f / rigidbodyPool.Get(a).mass : 0.0f;
				float invMassB = hasRbB ? 1.0f / rigidbodyPool.Get(b).mass : 0.0f;
				float invMassSum = invMassA + invMassB;

				if (invMassSum < 1e-8f) continue;

				tA.position -= info.normal * (info.depth * invMassA / invMassSum);
				tB.position += info.normal * (info.depth * invMassB / invMassSum);

				Vector3 relVel = { 0, 0, 0 };
				if (hasRbA) relVel = relVel + rigidbodyPool.Get(a).velocity;
				if (hasRbB) relVel = relVel - rigidbodyPool.Get(b).velocity;

				float velAlongNormal = Vector3::Dot(relVel, info.normal);
				if (velAlongNormal < 0) continue;

				float impulseScalar = -(1.0f + restitution) * velAlongNormal / invMassSum;
				Vector3 impulse = info.normal * impulseScalar;

				if (hasRbA) rigidbodyPool.Get(a).velocity += impulse * invMassA;
				if (hasRbB) rigidbodyPool.Get(b).velocity -= impulse * invMassB;
				}
			}
		}

	private:
		bool IsSupported(const TransformComponent& top, const TransformComponent& bottom, const Vector3& normal)
		{
			const TransformComponent& upper = (normal.y > 0) ? top : bottom;
			const TransformComponent& lower = (normal.y > 0) ? bottom : top;

			Vector3 lowerHalf = lower.scale * 0.5f;
			float supportMinX = lower.position.x - lowerHalf.x * supportThreshold;
			float supportMaxX = lower.position.x + lowerHalf.x * supportThreshold;
			float supportMinZ = lower.position.z - lowerHalf.z * supportThreshold;
			float supportMaxZ = lower.position.z + lowerHalf.z * supportThreshold;

			return upper.position.x >= supportMinX && upper.position.x <= supportMaxX &&
			       upper.position.z >= supportMinZ && upper.position.z <= supportMaxZ;
		}

		Vector3 RotateHalfExtents(const Vector3& half, const Matrix4& rot)
		{
			return {
				std::abs(rot.matrix[0][0]) * half.x + std::abs(rot.matrix[1][0]) * half.y + std::abs(rot.matrix[2][0]) * half.z,
				std::abs(rot.matrix[0][1]) * half.x + std::abs(rot.matrix[1][1]) * half.y + std::abs(rot.matrix[2][1]) * half.z,
				std::abs(rot.matrix[0][2]) * half.x + std::abs(rot.matrix[1][2]) * half.y + std::abs(rot.matrix[2][2]) * half.z
			};
		}

		float gravity = -9.8f;
		float restitution = 0.3f;
		float supportThreshold = 0.7f;
};

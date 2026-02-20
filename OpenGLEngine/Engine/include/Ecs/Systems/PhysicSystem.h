#pragma once
#include "Ecs/Components/RigidbodyComponent.h"
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Ecs/Core/EcsWorld.h"
#include <vector>

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
				Vector3 halfExtents = transform.scale * 0.5f;
				aabb.min = transform.position - halfExtents;
				aabb.max = transform.position + halfExtents;
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
		float gravity = -9.8f;
		float restitution = 0.3f;
};

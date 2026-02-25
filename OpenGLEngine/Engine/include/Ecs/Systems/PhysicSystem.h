#pragma once
#include "Ecs/Components/RigidbodyComponent.h"
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Ecs/Core/EcsWorld.h"
#include "Ecs/Core/ComponentPool.h"
#include <vector>
#include <cmath>
#include <algorithm>

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
					rigidbody.Integrate(transform.position, transform.rotation, transform.scale, deltaTime);
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

					const auto& aabbA = aabbPool.Get(a);
					const auto& aabbB = aabbPool.Get(b);

					CollisionInfo info = CheckCollision(aabbA, aabbB);
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

				Vector3 contactPoint = {
					((std::max)(aabbA.min.x, aabbB.min.x) + (std::min)(aabbA.max.x, aabbB.max.x)) * 0.5f,
					((std::max)(aabbA.min.y, aabbB.min.y) + (std::min)(aabbA.max.y, aabbB.max.y)) * 0.5f,
					((std::max)(aabbA.min.z, aabbB.min.z) + (std::min)(aabbA.max.z, aabbB.max.z)) * 0.5f
				};

				Vector3 rA = contactPoint - tA.position;
				Vector3 rB = contactPoint - tB.position;

				if (std::abs(info.normal.y) > 0.5f)
				{
					if (hasRbA)
					{
						auto& rbA = rigidbodyPool.Get(a);
						Vector3 sp = GetSupportPoint(tA, info.normal);
						Vector3 rFromPivot = tA.position - sp;
						Vector3 tipTorque = Vector3::Cross(rFromPivot, { 0, gravity * rbA.mass, 0 });
						float invI = 1.0f / rbA.GetInertia(tA.scale);
						rbA.angularVelocity = rbA.angularVelocity * contactAngularDamping
							+ tipTorque * (invI * deltaTime * tipTorqueScale);
					}
					if (hasRbB)
					{
						auto& rbB = rigidbodyPool.Get(b);
						Vector3 sp = GetSupportPoint(tB, info.normal * -1.0f);
						Vector3 rFromPivot = tB.position - sp;
						Vector3 tipTorque = Vector3::Cross(rFromPivot, { 0, gravity * rbB.mass, 0 });
						float invI = 1.0f / rbB.GetInertia(tB.scale);
						rbB.angularVelocity = rbB.angularVelocity * contactAngularDamping
							+ tipTorque * (invI * deltaTime * tipTorqueScale);
					}
				}

				Vector3 relVel = { 0, 0, 0 };
				if (hasRbA)
				{
					auto& rbA = rigidbodyPool.Get(a);
					relVel = relVel + rbA.velocity + Vector3::Cross(rbA.angularVelocity, rA);
				}
				if (hasRbB)
				{
					auto& rbB = rigidbodyPool.Get(b);
					relVel = relVel - (rbB.velocity + Vector3::Cross(rbB.angularVelocity, rB));
				}

				float velAlongNormal = Vector3::Dot(relVel, info.normal);
				if (velAlongNormal < 0) continue;

				float impulseScalar = -(1.0f + restitution) * velAlongNormal / invMassSum;
				Vector3 normalImpulse = info.normal * impulseScalar;

				if (hasRbA)
				{
					auto& rbA = rigidbodyPool.Get(a);
					rbA.velocity += normalImpulse * invMassA;
					float invInertiaA = 1.0f / rbA.GetInertia(tA.scale);
					rbA.angularVelocity += Vector3::Cross(rA, normalImpulse) * invInertiaA;
				}
				if (hasRbB)
				{
					auto& rbB = rigidbodyPool.Get(b);
					rbB.velocity -= normalImpulse * invMassB;
					float invInertiaB = 1.0f / rbB.GetInertia(tB.scale);
					rbB.angularVelocity -= Vector3::Cross(rB, normalImpulse) * invInertiaB;
				}

				Vector3 tangent = relVel - info.normal * velAlongNormal;
				float tangentLen = std::sqrt(Vector3::Dot(tangent, tangent));

				if (tangentLen > 1e-6f)
				{
					tangent = tangent / tangentLen;

					float jt = -Vector3::Dot(relVel, tangent) / invMassSum;
					float maxFriction = friction * std::abs(impulseScalar);
					jt = std::fmax(-maxFriction, std::fmin(maxFriction, jt));

					Vector3 frictionImpulse = tangent * jt;

					if (hasRbA)
					{
						auto& rbA = rigidbodyPool.Get(a);
						rbA.velocity += frictionImpulse * invMassA;
						float invInertiaA = 1.0f / rbA.GetInertia(tA.scale);
						rbA.angularVelocity += Vector3::Cross(rA, frictionImpulse) * invInertiaA;
					}
					if (hasRbB)
					{
						auto& rbB = rigidbodyPool.Get(b);
						rbB.velocity -= frictionImpulse * invMassB;
						float invInertiaB = 1.0f / rbB.GetInertia(tB.scale);
						rbB.angularVelocity -= Vector3::Cross(rB, frictionImpulse) * invInertiaB;
					}
				}
				}
			}
		}

	private:
		Vector3 GetSupportPoint(const TransformComponent& t, const Vector3& direction)
		{
			Vector3 half = t.scale * 0.5f;
			const auto& R = t.rotation;

			const float eps = 0.01f;
			float dx = R.matrix[0][0] * direction.x + R.matrix[0][1] * direction.y + R.matrix[0][2] * direction.z;
			float dy = R.matrix[1][0] * direction.x + R.matrix[1][1] * direction.y + R.matrix[1][2] * direction.z;
			float dz = R.matrix[2][0] * direction.x + R.matrix[2][1] * direction.y + R.matrix[2][2] * direction.z;

			float sx = (dx > eps) ? 1.0f : (dx < -eps) ? -1.0f : 0.0f;
			float sy = (dy > eps) ? 1.0f : (dy < -eps) ? -1.0f : 0.0f;
			float sz = (dz > eps) ? 1.0f : (dz < -eps) ? -1.0f : 0.0f;

			Vector3 corner(sx * half.x, sy * half.y, sz * half.z);

			return {
				t.position.x + corner.x * R.matrix[0][0] + corner.y * R.matrix[1][0] + corner.z * R.matrix[2][0],
				t.position.y + corner.x * R.matrix[0][1] + corner.y * R.matrix[1][1] + corner.z * R.matrix[2][1],
				t.position.z + corner.x * R.matrix[0][2] + corner.y * R.matrix[1][2] + corner.z * R.matrix[2][2]
			};
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
	float restitution = 0.1f;
	float friction = 0.4f;
	float tipTorqueScale = 20.0f;
	float contactAngularDamping = 0.92f;
};

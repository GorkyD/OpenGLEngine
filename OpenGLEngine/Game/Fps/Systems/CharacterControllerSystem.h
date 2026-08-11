#pragma once

#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"

class EcsWorld;

class CharacterControllerSystem
{
public:
    void Configure(EcsWorld& world, Entity entity);
    void Update(EcsWorld& world, const Vector3& wishDirection, bool wantsJump, bool sprinting, float deltaTime);

    bool IsGrounded() const
    {
        return grounded;
    }
    const Vector3& GetVelocity() const
    {
        return velocity;
    }
    bool IsMovingOnGround() const
    {
        return grounded && (velocity.x * velocity.x + velocity.z * velocity.z) > 0.05f;
    }

    float walkSpeed = 4.0f;
    float sprintMultiplier = 1.6f;
    float jumpSpeed = 5.0f;
    float gravity = -20.0f;
    float terminalVelocity = -50.0f;
    float stepHeight = 0.35f;
    float skinWidth = 0.02f;
    float groundSnapDistance = 0.25f;
    float maxSlopeNormalY = 0.5f;

private:
    Vector3 BoxCenter(EcsWorld& world) const;
    void SetBoxCenter(EcsWorld& world, const Vector3& center) const;

    void ResolveOverlaps(EcsWorld& world);
    Vector3 SweepAndSlide(EcsWorld& world, Vector3 center, Vector3 displacement, int maxIterations);
    bool TryStepUp(EcsWorld& world, Vector3& center, const Vector3& horizontalMove);
    void SnapToGround(EcsWorld& world, Vector3& center);

    Entity entity = INVALID_ENTITY;
    Vector3 halfExtents = {0.3f, 0.9f, 0.3f};
    Vector3 colliderOffset = {0, 0, 0};

    Vector3 velocity = {0, 0, 0};
    bool grounded = false;
};

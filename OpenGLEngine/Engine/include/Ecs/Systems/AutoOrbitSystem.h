#pragma once
#include <cmath>

#include "Ecs/Components/AutoOrbitComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Extension/Extension.h"
#include "Input/InputSystem.h"
#include "Math/Vector3.h"

class AutoOrbitSystem : public IEcsSystem
{
public:
    AutoOrbitSystem(InputSystem* input) : input(input) {}

    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& orbits = world.GetPool<AutoOrbitComponent>();
        auto& cameras = world.GetPool<CameraComponent>();
        auto& transforms = world.GetPool<TransformComponent>();

        const bool mouseRotating = input->IsLeftMouseDown() && (std::abs(input->GetMouseDeltaX()) > 0.01f ||
                                                                 std::abs(input->GetMouseDeltaY()) > 0.01f);
        const bool userInput = mouseRotating || input->IsKeyDown(Key::W) || input->IsKeyDown(Key::A) ||
                               input->IsKeyDown(Key::S) || input->IsKeyDown(Key::D) ||
                               input->IsKeyDown(Key::Space) || input->IsKeyDown(Key::LControl);

        for (auto& pair : orbits)
        {
            Entity entity = pair.first;
            auto& orbit = pair.second;

            if (!cameras.Has(entity) || !transforms.Has(entity))
                continue;

            auto& cam = cameras.Get(entity);
            auto& transform = transforms.Get(entity);

            if (userInput)
            {
                orbit.active = false;
                orbit.transitioning = false;
                orbit.idleTimer = 0.0f;
                continue;
            }

            if (!orbit.active && !orbit.transitioning)
            {
                orbit.idleTimer += deltaTime;
                if (orbit.idleTimer >= orbit.idleTimeout)
                {
                    orbit.transitioning = true;
                    orbit.transitionT = 0.0f;
                    orbit.transitionStartPos = transform.position;
                    orbit.transitionStartYaw = cam.yaw;
                    orbit.transitionStartPitch = cam.pitch;
                    orbit.angle = 0.0f;
                }
                continue;
            }

            if (orbit.transitioning)
            {
                orbit.transitionT += deltaTime / orbit.transitionDuration;
                const float t = orbit.transitionT >= 1.0f ? 1.0f : orbit.transitionT;
                const float smoothT = t * t * (3.0f - 2.0f * t);

                const Vector3 targetPos = OrbitPosition(orbit, orbit.angle);
                float targetYaw = 0.0f;
                float targetPitch = 0.0f;
                ComputeLookAtYawPitch(targetPos, orbit.center, targetYaw, targetPitch);

                transform.position = Lerp(orbit.transitionStartPos, targetPos, smoothT);
                cam.yaw = LerpAngle(orbit.transitionStartYaw, targetYaw, smoothT);
                cam.pitch = orbit.transitionStartPitch + (targetPitch - orbit.transitionStartPitch) * smoothT;

                if (orbit.transitionT >= 1.0f)
                {
                    orbit.transitioning = false;
                    orbit.active = true;
                }
                continue;
            }

            orbit.angle += orbit.angularSpeed * deltaTime;
            transform.position = OrbitPosition(orbit, orbit.angle);
            ComputeLookAtYawPitch(transform.position, orbit.center, cam.yaw, cam.pitch);
        }
    }

private:
    static Vector3 OrbitPosition(const AutoOrbitComponent& orbit, float angle)
    {
        return {orbit.center.x + std::sin(angle) * orbit.radius, orbit.height,
                orbit.center.z + std::cos(angle) * orbit.radius};
    }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return a + (b - a) * t;
    }

    static float LerpAngle(float a, float b, float t)
    {
        float delta = std::fmod(b - a + 3.14159265f, 6.28318531f);
        if (delta < 0.0f)
            delta += 6.28318531f;
        delta -= 3.14159265f;
        return a + delta * t;
    }

    static void ComputeLookAtYawPitch(const Vector3& fromPos, const Vector3& toPos, float& outYaw, float& outPitch)
    {
        const Vector3 dir = Vector3::Normalize(toPos - fromPos);
        outPitch = std::asin(dir.y);
        outYaw = std::atan2(dir.x, dir.z);
    }

    InputSystem* input;
};

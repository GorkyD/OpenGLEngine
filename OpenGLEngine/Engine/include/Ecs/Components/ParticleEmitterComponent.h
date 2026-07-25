#pragma once
#include <vector>

#include "Math/Vector3.h"
#include "Math/Vector4.h"

struct Particle
{
    Vector3 position;
    Vector3 velocity;
    float age = 0.0f;
    float lifetime = 1.0f;
};

struct ParticleEmitterComponent
{
    Vector3 localOffset = {0, 0, 0};
    Vector3 velocityMin = {-0.15f, 0.7f, -0.15f};
    Vector3 velocityMax = {0.15f, 1.2f, 0.15f};
    Vector3 gravity = {0, 0.15f, 0};
    float spawnRadius = 0.0f;

    float spawnRate = 12.0f;
    float lifetimeMin = 0.9f;
    float lifetimeMax = 1.6f;
    float startSize = 0.1f;
    float endSize = 0.4f;
    Vector4 color = {0.55f, 0.55f, 0.55f, 0.35f};
    unsigned int maxParticles = 48;
    bool additive = false;

    bool followCamera = false;
    Vector3 spawnAreaHalfExtents = {0, 0, 0};
    float widthScale = 1.0f;

    std::vector<Particle> particles;
    float spawnAccumulator = 0.0f;
};

#pragma once

#include <cmath>
#include <cstdlib>
#include <vector>
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/ParticleEmitterComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/UniformBuffer.h"
#include <glad/glad.h>

class ParticleSystem : public IEcsSystem
{
public:
    ParticleSystem(RenderEngine* re, ShaderProgramPtr shader, UniformBufferPtr ub) : renderEngine(re), particleShader(shader), uniformBuffer(ub) {}

    void Init(EcsWorld& world) override
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, bufferCapacity * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), static_cast<void*>(nullptr));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), reinterpret_cast<void*>(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    ~ParticleSystem() override
    {
        if (vbo)
            glDeleteBuffers(1, &vbo);
        if (vao)
            glDeleteVertexArrays(1, &vao);
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        if (!particleShader)
            return;

        auto& emitters = world.GetPool<ParticleEmitterComponent>();
        auto& transforms = world.GetPool<TransformComponent>();
        if (emitters.begin() == emitters.end())
            return;

        Vector3 cameraPos;
        for (const auto& [entityId, cameraComponent] : world.GetPool<CameraComponent>())
        {
            if (!cameraComponent.isActive || !transforms.Has(entityId))
                continue;
            cameraPos = transforms.Get(entityId).position;
            break;
        }

        for (auto& [entityId, emitterComponent] : emitters)
        {
            Vector3 origin = emitterComponent.localOffset;
            if (emitterComponent.followCamera)
                origin = cameraPos + emitterComponent.localOffset;
            else if (transforms.Has(entityId))
                origin = transforms.Get(entityId).position + emitterComponent.localOffset;

            SpawnParticles(emitterComponent, origin, deltaTime);
            UpdateParticles(emitterComponent, deltaTime);
        }

        Matrix4 identity;
        uniformBuffer->SetSubData(&identity, 0, sizeof(Matrix4));

        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);

        renderEngine->SetShaderProgram(particleShader);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        std::vector<ParticleVertex> normalVertices;
        std::vector<ParticleVertex> additiveVertices;

        for (auto& [entityId, emitterComponent] : emitters)
        {
            if (emitterComponent.particles.empty())
                continue;

            BuildBillboards(emitterComponent, cameraPos, emitterComponent.additive ? additiveVertices : normalVertices);
        }

        if (!normalVertices.empty())
        {
            EnsureCapacity(normalVertices.size());
            glBufferSubData(GL_ARRAY_BUFFER, 0, normalVertices.size() * sizeof(ParticleVertex), normalVertices.data());
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(normalVertices.size()));
        }

        if (!additiveVertices.empty())
        {
            EnsureCapacity(additiveVertices.size());
            glBufferSubData(GL_ARRAY_BUFFER, 0, additiveVertices.size() * sizeof(ParticleVertex), additiveVertices.data());
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(additiveVertices.size()));
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
    }

private:
    struct ParticleVertex
    {
        Vector3 position;
        float u, v;
        float r, g, b, a;
    };

    static float RandomRange(const float minValue, const float maxValue)
    {
        const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return minValue + t * (maxValue - minValue);
    }

    void SpawnParticles(ParticleEmitterComponent& emitter, const Vector3& origin, float deltaTime)
    {
        emitter.spawnAccumulator += emitter.spawnRate * deltaTime;

        while (emitter.spawnAccumulator >= 1.0f && emitter.particles.size() < emitter.maxParticles)
        {
            emitter.spawnAccumulator -= 1.0f;

            Particle particle;
            particle.position = origin;
            if (emitter.spawnRadius > 0.0f)
            {
                const float jitterAngle = RandomRange(0.0f, 6.2831853f);
                const float jitterRadius = RandomRange(0.0f, emitter.spawnRadius);
                particle.position.x += std::cos(jitterAngle) * jitterRadius;
                particle.position.z += std::sin(jitterAngle) * jitterRadius;
            }
            if (emitter.spawnAreaHalfExtents.x > 0.0f)
                particle.position.x += RandomRange(-emitter.spawnAreaHalfExtents.x, emitter.spawnAreaHalfExtents.x);
            if (emitter.spawnAreaHalfExtents.y > 0.0f)
                particle.position.y += RandomRange(-emitter.spawnAreaHalfExtents.y, emitter.spawnAreaHalfExtents.y);
            if (emitter.spawnAreaHalfExtents.z > 0.0f)
                particle.position.z += RandomRange(-emitter.spawnAreaHalfExtents.z, emitter.spawnAreaHalfExtents.z);

            particle.velocity = {RandomRange(emitter.velocityMin.x, emitter.velocityMax.x), RandomRange(emitter.velocityMin.y, emitter.velocityMax.y), RandomRange(emitter.velocityMin.z, emitter.velocityMax.z)};
            particle.lifetime = RandomRange(emitter.lifetimeMin, emitter.lifetimeMax);
            particle.age = 0.0f;
            emitter.particles.push_back(particle);
        }
    }

    void UpdateParticles(ParticleEmitterComponent& emitter, const float deltaTime)
    {
        for (size_t i = 0; i < emitter.particles.size();)
        {
            Particle& particle = emitter.particles[i];
            particle.age += deltaTime;
            if (particle.age >= particle.lifetime)
            {
                particle.position = emitter.particles.back().position;
                particle.velocity = emitter.particles.back().velocity;
                particle.age = emitter.particles.back().age;
                particle.lifetime = emitter.particles.back().lifetime;
                emitter.particles.pop_back();
                continue;
            }

            particle.velocity += emitter.gravity * deltaTime;
            particle.position += particle.velocity * deltaTime;
            i++;
        }
    }

    void BuildBillboards(const ParticleEmitterComponent& emitter, const Vector3& cameraPos, std::vector<ParticleVertex>& outVertices)
    {
        outVertices.reserve(outVertices.size() + emitter.particles.size() * 6);

        for (const auto& particle : emitter.particles)
        {
            const float t = particle.lifetime > 0.0f ? particle.age / particle.lifetime : 1.0f;
            const float size = emitter.startSize + (emitter.endSize - emitter.startSize) * t;
            const float alpha = emitter.color.w * (1.0f - t);

            Vector3 toCamera = cameraPos - particle.position;
            const float toCameraLen = std::sqrt(Vector3::Dot(toCamera, toCamera));
            if (toCameraLen < 1e-4f)
                toCamera = {0, 0, 1};
            else
                toCamera = toCamera * (1.0f / toCameraLen);

            Vector3 worldUp = {0, 1, 0};
            Vector3 right = Vector3::Cross(worldUp, toCamera);
            const float rightLen = std::sqrt(Vector3::Dot(right, right));
            right = rightLen > 1e-4f ? right * (1.0f / rightLen) : Vector3(1, 0, 0);
            Vector3 up = Vector3::Cross(toCamera, right);

            const float half = size * 0.5f;
            const Vector3 rightOffset = right * (half * emitter.widthScale);
            const Vector3 upOffset = up * half;

            const Vector3 bl = particle.position - rightOffset - upOffset;
            const Vector3 br = particle.position + rightOffset - upOffset;
            const Vector3 tr = particle.position + rightOffset + upOffset;
            const Vector3 tl = particle.position - rightOffset + upOffset;

            outVertices.push_back({bl, 0.0f, 0.0f, emitter.color.x, emitter.color.y, emitter.color.z, alpha});
            outVertices.push_back({br, 1.0f, 0.0f, emitter.color.x, emitter.color.y, emitter.color.z, alpha});
            outVertices.push_back({tr, 1.0f, 1.0f, emitter.color.x, emitter.color.y, emitter.color.z, alpha});

            outVertices.push_back({bl, 0.0f, 0.0f, emitter.color.x, emitter.color.y, emitter.color.z, alpha});
            outVertices.push_back({tr, 1.0f, 1.0f, emitter.color.x, emitter.color.y, emitter.color.z, alpha});
            outVertices.push_back({tl, 0.0f, 1.0f, emitter.color.x, emitter.color.y, emitter.color.z, alpha});
        }
    }

    void EnsureCapacity(size_t vertexCount)
    {
        if (vertexCount <= bufferCapacity)
            return;

        bufferCapacity = vertexCount * 2;
        glBufferData(GL_ARRAY_BUFFER, bufferCapacity * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);
    }

    RenderEngine* renderEngine;
    ShaderProgramPtr particleShader;
    UniformBufferPtr uniformBuffer;

    size_t bufferCapacity = 512;

    unsigned int vao = 0;
    unsigned int vbo = 0;
};

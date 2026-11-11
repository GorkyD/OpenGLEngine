#pragma once

#include <cstddef>
#include "Math/Matrix4.h"

struct UniformData
{
    Matrix4 world;
    Matrix4 view;
    Matrix4 projection;
};

constexpr int MaxFrameLights = 32;
constexpr unsigned int FrameUniformBufferSlot = 1;

struct FrameUniformData
{
    Matrix4 lightSpaceMatrix;
    float lightColorIntensity[MaxFrameLights][4];
    float lightPositionRange[MaxFrameLights][4];
    float lightDirectionType[MaxFrameLights][4];
    float lightCone[MaxFrameLights][4];
    float viewPos[3];
    float ambientIntensity;
    float ambientColor[3];
    float fogStart;
    float fogColor[3];
    float fogEnd;
    float fogRadialOrigin[3];
    float fogRadialStart;
    float fogRadialEnd;
    float fogGlowExponent;
    float fogGlowStrength;
    int fogRadialEnabled;
    int numLights;
    int shadowLightIndex;
    float shadowBias;
    float shadowAmbientOcclusion;
    float shadowNormalBias;
    float padding[3];
};

static_assert(sizeof(Matrix4) == 64);
static_assert(offsetof(FrameUniformData, lightColorIntensity) == 64);
static_assert(offsetof(FrameUniformData, viewPos) == 64 + MaxFrameLights * 16 * 4);
static_assert(offsetof(FrameUniformData, fogRadialEnd) == offsetof(FrameUniformData, viewPos) + 64);
static_assert(offsetof(FrameUniformData, shadowNormalBias) == offsetof(FrameUniformData, viewPos) + 96);
static_assert(sizeof(FrameUniformData) % 16 == 0);

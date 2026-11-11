#pragma once

constexpr unsigned int InstanceAttributeBaseLocation = 8;

struct InstanceData
{
    float world[16];
    float diffuseColor[4];
    float emissiveColorIntensity[4];
    float materialParams[4];
};

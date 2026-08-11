#pragma once

#include "Extension/Extension.h"

class RenderEngine;
struct TerrainComponent;

class TerrainGenerator
{
public:
    struct Params
    {
        int resolution = 128;
        float worldSize = 100.0f;
        float heightScale = 8.0f;
        float noiseFrequency = 0.015f;
        int octaves = 4;
        float persistence = 0.5f;
        int seed = 1337;
        float uvTiling = 20.0f;
    };

    static void Generate(RenderEngine* renderEngine, const Params& params, TerrainComponent& outTerrain);
};

#pragma once

class Engine;

class EnvironmentBuilder
{
public:
    static void Build(Engine& engine);

private:
    static void CreateProceduralSky(Engine& engine);
    static void CreateTerrain(Engine& engine);
    static void CreateAtmosphere(Engine& engine);
};

#include "Systems/EnvironmentBuilder.h"
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/FogComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/PostProcessComponent.h"
#include "Ecs/Components/ProceduralSkyComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TerrainComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Render/RenderEngine.h"
#include "Render/Texture.h"
#include "Resource/TerrainGenerator.h"

void EnvironmentBuilder::Build(Engine& engine)
{
    CreateProceduralSky(engine);
    CreateTerrain(engine);
    CreateAtmosphere(engine);
}

void EnvironmentBuilder::CreateProceduralSky(Engine& engine)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();

    static const float cubeVertices[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
    };

    VertexAttributes attrs[] = {{3}};
    const auto vao = renderEngine->CreateVertexArrayObject({(void*)cubeVertices, 3 * sizeof(float), 36, attrs, 1});

    const auto skyEntity = world.CreateEntity();
    auto& sky = world.AddComponent<ProceduralSkyComponent>(skyEntity);
    sky.shader = engine.GetShaders().skyProcedural;
    sky.vao = vao;
    sky.moonTexture = Texture::LoadFromFile("Assets/Textures/Sky/moon_albedo.jpg");
    sky.zenithColor = {0.015f, 0.02f, 0.05f};
    sky.horizonColor = {0.05f, 0.06f, 0.12f};
    sky.groundColor = {0.01f, 0.01f, 0.02f};
}

void EnvironmentBuilder::CreateTerrain(Engine& engine)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();
    const auto& shaders = engine.GetShaders();

    TerrainGenerator::Params params;
    params.resolution = 128;
    params.worldSize = 150.0f;
    params.heightScale = 6.0f;
    params.noiseFrequency = 0.015f;
    params.octaves = 4;
    params.persistence = 0.5f;
    params.seed = 1337;
    params.uvTiling = 30.0f;

    const auto terrainEntity = world.CreateEntity();
    world.AddComponent<NameComponent>(terrainEntity).name = "Terrain";
    world.AddComponent<TransformComponent>(terrainEntity);

    auto& terrain = world.AddComponent<TerrainComponent>(terrainEntity);
    TerrainGenerator::Generate(renderEngine, params, terrain);

    auto& mesh = world.AddComponent<MeshComponent>(terrainEntity);
    mesh.vao = terrain.vao;
    mesh.indexCount = terrain.indexCount;
    mesh.castsShadow = true;

    auto& material = world.AddComponent<MaterialComponent>(terrainEntity);
    material.diffuseTexture = Texture::LoadFromFile("Assets/Textures/PBR/grass/albedo.png");
    material.normalTexture = Texture::LoadFromFile("Assets/Textures/PBR/grass/normal.png");
    material.roughnessTexture = Texture::LoadFromFile("Assets/Textures/PBR/grass/roughness.png");
    material.metallicTexture = Texture::LoadFromFile("Assets/Textures/PBR/grass/metallic.png");
    material.aoTexture = Texture::LoadFromFile("Assets/Textures/PBR/grass/ao.png");

    auto& shaderComp = world.AddComponent<ShaderComponent>(terrainEntity);
    shaderComp.shader = shaders.lit;
    shaderComp.shaderType = ShaderRenderType::Lit;
}

void EnvironmentBuilder::CreateAtmosphere(Engine& engine)
{
    auto& world = engine.GetWorld();

    const auto fogEntity = world.CreateEntity();
    auto& fog = world.AddComponent<FogComponent>(fogEntity);
    fog.color = {0.04f, 0.05f, 0.09f};
    fog.start = 8.0f;
    fog.end = 38.0f;
    fog.radialEnabled = true;
    fog.radialOrigin = {0.0f, 0.0f, 0.0f};
    fog.radialStart = 45.0f;
    fog.radialEnd = 68.0f;

    const auto postProcessEntity = world.CreateEntity();
    auto& postProcess = world.AddComponent<PostProcessComponent>(postProcessEntity);
    postProcess.exposure = 1.1f;
    postProcess.vignetteStrength = 0.45f;
    postProcess.vignetteRadius = 0.7f;
    postProcess.vignetteSoftness = 0.5f;

    const auto ambientEntity = world.CreateEntity();
    auto& ambient = world.AddComponent<AmbientLightComponent>(ambientEntity);
    ambient.color = {0.16f, 0.2f, 0.32f};
    ambient.intensity = 0.55f;

    const auto lightEntity = world.CreateEntity();
    auto& light = world.AddComponent<LightComponent>(lightEntity);
    light.type = LightType::Directional;
    light.color = {0.55f, 0.65f, 0.9f};
    light.direction = {0.4f, 1.0f, -0.3f};
    light.intensity = 0.55f;
    light.castShadows = true;
    light.shadowOrthoSize = 12.0f;
    light.shadowDistance = 35.0f;
    light.shadowFocusDistance = 9.0f;
    light.shadowBias = 0.01f;
    light.shadowAmbientOcclusion = 0.6f;
    light.shadowNormalBias = 0.1f;
}

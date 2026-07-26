#include "Scene/DemoScene.h"
#include <array>
#include <cmath>
#include <vector>
#include "Engine/Engine.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/Font.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/Texture.h"
#include "Resource/EntityFactory.h"
#include "Resource/MeshFactory.h"
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/AutoOrbitComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/FogComponent.h"
#include "Ecs/Components/FpsControlComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ParticleEmitterComponent.h"
#include "Ecs/Components/PendulumComponent.h"
#include "Ecs/Components/PickableComponent.h"
#include "Ecs/Components/RotatorComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/SkyboxComponent.h"
#include "Ecs/Components/TextComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"

void DemoScene::OnLoad(Engine& engine)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();
    auto* audioSystem = engine.GetAudioSystem();
    const auto& shaders = engine.GetShaders();

    audioSystem->SetMasterVolume(100.0f);
    audioSystem->SetFireVolume(100.0f);
    audioSystem->SetAmbientVolume(100.0f);
    audioSystem->PlayEvent("Play_Ambient", AudioSystem::AmbientId);

    CreateUiText(engine);
    CreateSkybox(engine, shaders.skybox);

    const auto ambientEntity = world.CreateEntity();
    auto& ambient = world.AddComponent<AmbientLightComponent>(ambientEntity);
    ambient.color = {0.55f, 0.45f, 0.2f};
    ambient.intensity = 0.2f;

    const auto fogEntity = world.CreateEntity();
    auto& fog = world.AddComponent<FogComponent>(fogEntity);
    fog.color = {0.35f, 0.38f, 0.45f};
    fog.start = 13.0f;
    fog.end = 45.0f;
    fog.horizonSpread = 0.1f;
    fog.horizonIntensity = 0.85f;

    const auto moonEntity = world.CreateEntity();
    auto& moon = world.AddComponent<LightComponent>(moonEntity);
    moon.type = LightType::Directional;
    moon.color = {1.0f, 0.85f, 0.4f};
    moon.intensity = 0.7f;
    moon.direction = {0.4f, 1.0f, -0.2f};

    cameraEntity = world.CreateEntity();
    auto& camTransform = world.AddComponent<TransformComponent>(cameraEntity);
    camTransform.position = {0, 1.0f, -3.0f};
    world.AddComponent<CameraComponent>(cameraEntity);
    world.AddComponent<FpsControllerComponent>(cameraEntity);

    auto floorEntity = EntityFactory::CreateModelEntity(world, renderEngine, "Assets/Models/cube.obj", shaders.lit);
    auto& floorTransform = world.GetComponent<TransformComponent>(floorEntity);
    floorTransform.position = {0, -2.0f, 0};
    floorTransform.scale = {200.0f, 0.5f, 200.0f};
    auto& floorMat = world.GetComponent<MaterialComponent>(floorEntity);
    floorMat.diffuseTexture = Texture::LoadFromFile("Assets/Textures/Marble.jpg");
    floorMat.diffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    auto& floorShaderComp = world.GetComponent<ShaderComponent>(floorEntity);
    floorShaderComp.shaderType = ShaderRenderType::Lit;

    const float platformTopY = floorTransform.position.y + floorTransform.scale.y * 0.5f;

    CreateMaterialGallery(engine, shaders.lit, platformTopY, floorTransform.scale.x * 0.5f);

    CreateHangingLamp(engine, {0.0f, platformTopY + 6.5f, 0.0f}, shaders.lit);

    CreateRain(engine);

    auto& orbit = world.AddComponent<AutoOrbitComponent>(cameraEntity);
    orbit.center = {0.0f, platformTopY + 0.6f, 0.0f};
    orbit.radius = 11.0f;
    orbit.height = platformTopY + 3.6f;
    orbit.angularSpeed = 0.2f;
    orbit.idleTimeout = 4.0f;
    orbit.transitionDuration = 1.5f;
}

void DemoScene::CreateRain(Engine& engine)
{
    auto& world = engine.GetWorld();

    const auto rainEntity = world.CreateEntity();
    auto& rain = world.AddComponent<ParticleEmitterComponent>(rainEntity);
    rain.followCamera = true;
    rain.localOffset = {0.0f, 14.0f, 0.0f};
    rain.spawnAreaHalfExtents = {25.0f, 0.0f, 25.0f};
    rain.velocityMin = {-0.4f, -16.0f, -0.4f};
    rain.velocityMax = {0.4f, -13.0f, 0.4f};
    rain.gravity = {0.0f, 0.0f, 0.0f};
    rain.spawnRate = 260.0f;
    rain.lifetimeMin = 1.3f;
    rain.lifetimeMax = 1.6f;
    rain.startSize = 0.6f;
    rain.endSize = 0.6f;
    rain.widthScale = 0.05f;
    rain.color = {0.7f, 0.75f, 0.85f, 0.5f};
    rain.maxParticles = 500;
    rain.additive = false;
}

void DemoScene::CreateSkybox(Engine& engine, ShaderProgramPtr shader)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();

    const std::array<std::string, 6> faces = {
        "Assets/Textures/Skybox/right.jpg", "Assets/Textures/Skybox/left.jpg", "Assets/Textures/Skybox/top.jpg", "Assets/Textures/Skybox/bottom.jpg", "Assets/Textures/Skybox/front.jpg", "Assets/Textures/Skybox/back.jpg",
    };
    auto cubemap = Texture::LoadCubemap(faces);
    if (!cubemap)
        return;

    static const float skyboxVertices[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
    };

    VertexAttributes attrs[] = {{3}};
    const auto vao = renderEngine->CreateVertexArrayObject({(void*)skyboxVertices, 3 * sizeof(float), 36, attrs, 1});

    const auto skyboxEntity = world.CreateEntity();
    auto& skybox = world.AddComponent<SkyboxComponent>(skyboxEntity);
    skybox.shader = shader;
    skybox.cubemap = cubemap;
    skybox.vao = vao;
}

void DemoScene::CreateTorch(Engine& engine, const Vector3& basePosition, ShaderProgramPtr handleShader, ShaderProgramPtr fireShader)
{
    auto& world = engine.GetWorld();
    auto* audioSystem = engine.GetAudioSystem();

    const float handleHeight = 1.2f;
    const float handleRadius = 0.12f;

    auto handleEntity = EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), "Assets/Models/cube.obj", handleShader);
    auto& handleTransform = world.GetComponent<TransformComponent>(handleEntity);
    handleTransform.position = basePosition + Vector3(0.0f, handleHeight * 0.5f, 0.0f);
    handleTransform.scale = {handleRadius, handleHeight, handleRadius};
    auto& handleMat = world.GetComponent<MaterialComponent>(handleEntity);
    handleMat.diffuseTexture = nullptr;
    handleMat.diffuseColor = {0.35f, 0.22f, 0.12f, 1.0f};
    auto& handleShaderComp = world.GetComponent<ShaderComponent>(handleEntity);
    handleShaderComp.shaderType = ShaderRenderType::Lit;

    unsigned int flameIndexCount = 0;
    const auto flameVao = CreateFlameMesh(engine, flameIndexCount);

    const auto flameEntity = world.CreateEntity();
    auto& flameTransform = world.AddComponent<TransformComponent>(flameEntity);
    flameTransform.position = basePosition + Vector3(0.0f, handleHeight, 0.0f);
    flameTransform.scale = {0.18f, 0.5f, 0.18f};

    auto& flameMesh = world.AddComponent<MeshComponent>(flameEntity);
    flameMesh.vao = flameVao;
    flameMesh.indexCount = flameIndexCount;

    auto& flameShaderComp = world.AddComponent<ShaderComponent>(flameEntity);
    flameShaderComp.shader = fireShader;
    flameShaderComp.shaderType = ShaderRenderType::Fire;

    const auto flameLightEntity = world.CreateEntity();
    auto& flameLight = world.AddComponent<LightComponent>(flameLightEntity);
    flameLight.type = LightType::Point;
    flameLight.color = {1.0f, 0.55f, 0.15f};
    flameLight.intensity = 5.0f;
    flameLight.position = flameTransform.position + Vector3(0.0f, flameTransform.scale.y * 0.3f, 0.0f);
    flameLight.range = 14.0f;

    auto& smoke = world.AddComponent<ParticleEmitterComponent>(flameEntity);
    smoke.localOffset = {0.0f, flameTransform.scale.y * 0.9f, 0.0f};
    smoke.spawnRadius = 0.15f;
    smoke.velocityMin = {-0.5f, 0.5f, -0.5f};
    smoke.velocityMax = {0.5f, 1.1f, 0.5f};
    smoke.gravity = {0.0f, 0.05f, 0.0f};
    smoke.lifetimeMin = 1.4f;
    smoke.lifetimeMax = 2.4f;
    smoke.startSize = 0.15f;
    smoke.endSize = 0.9f;
    smoke.color = {0.5f, 0.5f, 0.5f, 0.22f};

    const auto sparksEntity = world.CreateEntity();
    auto& sparksTransform = world.AddComponent<TransformComponent>(sparksEntity);
    sparksTransform.position = flameTransform.position;

    auto& sparks = world.AddComponent<ParticleEmitterComponent>(sparksEntity);
    sparks.localOffset = {0.0f, flameTransform.scale.y * 0.4f, 0.0f};
    sparks.velocityMin = {-0.6f, 1.0f, -0.6f};
    sparks.velocityMax = {0.6f, 2.4f, 0.6f};
    sparks.gravity = {0.0f, -2.5f, 0.0f};
    sparks.spawnRate = 10.0f;
    sparks.lifetimeMin = 0.3f;
    sparks.lifetimeMax = 0.7f;
    sparks.startSize = 0.05f;
    sparks.endSize = 0.02f;
    sparks.color = {1.0f, 0.6f, 0.1f, 1.0f};
    sparks.maxParticles = 24;
    sparks.additive = true;

    audioSystem->RegisterGameObject(flameEntity, "Torch");
    audioSystem->SetPosition(flameEntity, flameTransform.position.x, flameTransform.position.y, flameTransform.position.z);
    audioSystem->PlayEvent("Play_Fire", flameEntity);
}

void DemoScene::CreateHangingLamp(Engine& engine, const Vector3& pivot, ShaderProgramPtr shader)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();

    constexpr float chainLength = 2.5f;
    constexpr float amplitude = 0.3f;
    constexpr float speed = 1.4f;

    auto chainEntity = EntityFactory::CreateModelEntity(world, renderEngine, "Assets/Models/cube.obj", shader);
    auto& chainTransform = world.GetComponent<TransformComponent>(chainEntity);
    chainTransform.scale = {0.1f, chainLength, 0.1f};
    auto& chainMat = world.GetComponent<MaterialComponent>(chainEntity);
    chainMat.diffuseTexture = nullptr;
    chainMat.diffuseColor = {0.15f, 0.15f, 0.17f, 1.0f};
    auto& chainShaderComp = world.GetComponent<ShaderComponent>(chainEntity);
    chainShaderComp.shaderType = ShaderRenderType::Lit;

    auto& chainPendulum = world.AddComponent<PendulumComponent>(chainEntity);
    chainPendulum.pivot = pivot;
    chainPendulum.armLength = chainLength * 0.5f;
    chainPendulum.amplitudeRadians = amplitude;
    chainPendulum.speed = speed;

    unsigned int shadeIndexCount = 0;
    const auto shadeVao = MeshFactory::CreateSphere(renderEngine, shadeIndexCount, 12, 18, 0.35f);

    const auto lampEntity = world.CreateEntity();
    auto& lampTransform = world.AddComponent<TransformComponent>(lampEntity);
    lampTransform.position = pivot + Vector3(0.0f, -chainLength, 0.0f);

    auto& lampMesh = world.AddComponent<MeshComponent>(lampEntity);
    lampMesh.vao = shadeVao;
    lampMesh.indexCount = shadeIndexCount;

    auto& lampMat = world.AddComponent<MaterialComponent>(lampEntity);
    lampMat.diffuseTexture = nullptr;
    lampMat.diffuseColor = {1.0f, 0.85f, 0.55f, 1.0f};
    lampMat.emissiveColor = {1.0f, 0.75f, 0.4f};
    lampMat.emissiveIntensity = 3.0f;

    auto& lampShaderComp = world.AddComponent<ShaderComponent>(lampEntity);
    lampShaderComp.shader = shader;
    lampShaderComp.shaderType = ShaderRenderType::Lit;

    auto& lampLight = world.AddComponent<LightComponent>(lampEntity);
    lampLight.type = LightType::Point;
    lampLight.color = {1.0f, 0.8f, 0.5f};
    lampLight.intensity = 6.0f;
    lampLight.range = 14.0f;
    lampLight.position = lampTransform.position;

    auto& lampPendulum = world.AddComponent<PendulumComponent>(lampEntity);
    lampPendulum.pivot = pivot;
    lampPendulum.armLength = chainLength;
    lampPendulum.amplitudeRadians = amplitude;
    lampPendulum.speed = speed;
    lampPendulum.lightBaseIntensity = lampLight.intensity;
    lampPendulum.emissiveBaseIntensity = lampMat.emissiveIntensity;
    lampPendulum.flickerAmount = 0.12f;
    lampPendulum.flickerSpeed = 5.0f;
}

VertexArrayObjectPtr DemoScene::CreateFlameMesh(Engine& engine, unsigned int& outIndexCount)
{
    auto* renderEngine = engine.GetRenderEngine();

    constexpr int segments = 10;

    struct RingProfile
    {
        float height;
        float radius;
    };
    static const RingProfile profile[] = {
        {0.0f, 0.42f}, {0.16f, 0.78f}, {0.36f, 0.66f}, {0.58f, 0.4f}, {0.8f, 0.16f}, {1.0f, 0.0f},
    };
    constexpr int ringCount = sizeof(profile) / sizeof(profile[0]);

    std::vector<std::vector<Vector3>> rings(ringCount);
    for (int r = 0; r < ringCount; r++)
    {
        rings[r].resize(segments);
        for (int s = 0; s < segments; s++)
        {
            const float angle = (2.0f * 3.14159265f * static_cast<float>(s)) / static_cast<float>(segments);
            rings[r][s] = {std::cos(angle) * profile[r].radius, profile[r].height, std::sin(angle) * profile[r].radius};
        }
    }

    std::vector<Vector3> vertices;
    vertices.reserve(segments * (ringCount - 1) * 6);

    for (int r = 0; r < ringCount - 1; r++)
    {
        for (int s = 0; s < segments; s++)
        {
            const Vector3& a = rings[r][s];
            const Vector3& b = rings[r][(s + 1) % segments];

            if (profile[r + 1].radius < 0.001f)
            {
                vertices.push_back(a);
                vertices.push_back(b);
                vertices.push_back(rings[r + 1][0]);
                continue;
            }

            const Vector3& c = rings[r + 1][s];
            const Vector3& d = rings[r + 1][(s + 1) % segments];

            vertices.push_back(a);
            vertices.push_back(b);
            vertices.push_back(d);

            vertices.push_back(a);
            vertices.push_back(d);
            vertices.push_back(c);
        }
    }

    const Vector3 baseCenter = {0.0f, 0.0f, 0.0f};
    for (int s = 0; s < segments; s++)
    {
        const Vector3& a = rings[0][s];
        const Vector3& b = rings[0][(s + 1) % segments];

        vertices.push_back(baseCenter);
        vertices.push_back(b);
        vertices.push_back(a);
    }

    std::vector<unsigned int> indices(vertices.size());
    for (unsigned int i = 0; i < indices.size(); i++)
        indices[i] = i;

    VertexAttributes attrs[] = {{3}};
    const auto vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), 3 * sizeof(float), static_cast<int>(vertices.size()), attrs, 1},
                                                           {static_cast<void*>(indices.data()), static_cast<int>(indices.size() * sizeof(unsigned int))});

    outIndexCount = static_cast<unsigned int>(indices.size());
    return vao;
}

void DemoScene::CreateMaterialGallery(Engine& engine, ShaderProgramPtr shader, float platformTopY, float platformHalfWidth)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();

    static const std::array<std::string, 5> materials = {"gold", "grass", "plastic", "rusted_iron", "wall"};
    constexpr float sphereRadius = 0.6f;
    constexpr float spacing = 2.2f;
    constexpr float torchOffsetX = 6.5f;
    constexpr float torchOffsetZ = 3.5f;

    const float startX = -spacing * (static_cast<float>(materials.size() - 1) * 0.5f);

    const auto& shaders = engine.GetShaders();
    CreateTorch(engine, {torchOffsetX, platformTopY, torchOffsetZ}, shader, shaders.fire);
    CreateTorch(engine, {-torchOffsetX, platformTopY, torchOffsetZ}, shader, shaders.fire);
    CreateTorch(engine, {torchOffsetX, platformTopY, -torchOffsetZ}, shader, shaders.fire);
    CreateTorch(engine, {-torchOffsetX, platformTopY, -torchOffsetZ}, shader, shaders.fire);

    for (size_t i = 0; i < materials.size(); i++)
    {
        unsigned int indexCount = 0;
        const auto vao = MeshFactory::CreateSphere(renderEngine, indexCount, 16, 24, sphereRadius);

        const auto entity = world.CreateEntity();
        auto& transform = world.AddComponent<TransformComponent>(entity);
        transform.position = {startX + spacing * static_cast<float>(i), platformTopY + sphereRadius, 0.0f};

        auto& mesh = world.AddComponent<MeshComponent>(entity);
        mesh.vao = vao;
        mesh.indexCount = indexCount;

        auto& material = world.AddComponent<MaterialComponent>(entity);
        material.diffuseTexture = Texture::LoadFromFile("Assets/Textures/PBR/" + materials[i] + "/albedo.png");
        material.normalTexture = Texture::LoadFromFile("Assets/Textures/PBR/" + materials[i] + "/normal.png");
        material.roughnessTexture = Texture::LoadFromFile("Assets/Textures/PBR/" + materials[i] + "/roughness.png");
        material.metallicTexture = Texture::LoadFromFile("Assets/Textures/PBR/" + materials[i] + "/metallic.png");
        material.aoTexture = Texture::LoadFromFile("Assets/Textures/PBR/" + materials[i] + "/ao.png");

        auto& shaderComp = world.AddComponent<ShaderComponent>(entity);
        shaderComp.shader = shader;
        shaderComp.shaderType = ShaderRenderType::Lit;

        auto& rotator = world.AddComponent<RotatorComponent>(entity);
        rotator.degreesPerSecond = 4.0f + 2.0f * static_cast<float>(i);

        auto& pickable = world.AddComponent<PickableComponent>(entity);
        pickable.name = materials[i];
        pickable.radius = sphereRadius;
    }
}

void DemoScene::CreateUiText(Engine& engine)
{
    auto& world = engine.GetWorld();

    auto font = Font::LoadFromFile("Assets/Fonts/JetBrainsMono-Regular.ttf", 32.0f);
    if (!font)
        return;

    const auto titleEntity = world.CreateEntity();
    auto& title = world.AddComponent<TextComponent>(titleEntity);
    title.font = font;
    title.text = "OpenGLEngine";
    title.position = {16.0f, 16.0f};
    title.color = {1.0f, 1.0f, 1.0f, 1.0f};
    title.scale = 1.0f;

    fpsTextEntity = world.CreateEntity();
    auto& fpsText = world.AddComponent<TextComponent>(fpsTextEntity);
    fpsText.font = font;
    fpsText.text = "FPS: 0";
    fpsText.position = {16.0f, 56.0f};
    fpsText.color = {0.4f, 1.0f, 0.4f, 1.0f};
    fpsText.scale = 0.75f;
}

void DemoScene::OnUpdate(Engine& engine, float deltaTime)
{
    auto& world = engine.GetWorld();
    auto* audioSystem = engine.GetAudioSystem();

    if (cameraEntity != 0)
    {
        const auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);
        audioSystem->SetListenerPosition(camTransform.position.x, camTransform.position.y, camTransform.position.z);
    }

    if (fpsTextEntity == 0)
        return;

    fpsUpdateTimer += deltaTime;
    fpsFrameCount++;
    if (fpsUpdateTimer < 0.2f)
        return;

    auto& fpsText = world.GetComponent<TextComponent>(fpsTextEntity);
    const int fps = fpsUpdateTimer > 0.0f ? static_cast<int>((float)fpsFrameCount / fpsUpdateTimer) : 0;
    fpsText.text = "FPS: " + std::to_string(fps);

    fpsUpdateTimer = 0.0f;
    fpsFrameCount = 0;
}

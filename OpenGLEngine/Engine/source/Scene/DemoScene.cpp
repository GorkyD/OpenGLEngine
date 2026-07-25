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
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/FpsControlComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/PendulumComponent.h"
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
}

void DemoScene::CreateSkybox(Engine& engine, ShaderProgramPtr shader)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();

    const std::array<std::string, 6> faces = {
        "Assets/Textures/Skybox/right.jpg", "Assets/Textures/Skybox/left.jpg",
        "Assets/Textures/Skybox/top.jpg",   "Assets/Textures/Skybox/bottom.jpg",
        "Assets/Textures/Skybox/front.jpg", "Assets/Textures/Skybox/back.jpg",
    };
    auto cubemap = Texture::LoadCubemap(faces);
    if (!cubemap)
        return;

    static const float skyboxVertices[] = {
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
    };

    VertexAttributes attrs[] = {{3}};
    const auto vao = renderEngine->CreateVertexArrayObject(
        {(void*)skyboxVertices, 3 * sizeof(float), 36, attrs, 1});

    const auto skyboxEntity = world.CreateEntity();
    auto& skybox = world.AddComponent<SkyboxComponent>(skyboxEntity);
    skybox.shader = shader;
    skybox.cubemap = cubemap;
    skybox.vao = vao;
}

void DemoScene::CreateTorch(Engine& engine, const Vector3& basePosition, ShaderProgramPtr handleShader,
                            ShaderProgramPtr fireShader)
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

    constexpr int segments = 8;

    std::vector<Vector3> basePoints(segments);
    for (int i = 0; i < segments; i++)
    {
        const float angle = (2.0f * 3.14159265f * static_cast<float>(i)) / static_cast<float>(segments);
        basePoints[i] = {std::cos(angle), 0.0f, std::sin(angle)};
    }

    const Vector3 apex = {0.0f, 1.0f, 0.0f};
    const Vector3 baseCenter = {0.0f, 0.0f, 0.0f};

    std::vector<Vector3> vertices;
    vertices.reserve(segments * 6);
    for (int i = 0; i < segments; i++)
    {
        const Vector3& a = basePoints[i];
        const Vector3& b = basePoints[(i + 1) % segments];

        vertices.push_back(apex);
        vertices.push_back(a);
        vertices.push_back(b);

        vertices.push_back(baseCenter);
        vertices.push_back(b);
        vertices.push_back(a);
    }

    std::vector<unsigned int> indices(vertices.size());
    for (unsigned int i = 0; i < indices.size(); i++)
        indices[i] = i;

    VertexAttributes attrs[] = {{3}};
    const auto vao = renderEngine->CreateVertexArrayObject(
        {static_cast<void*>(vertices.data()), 3 * sizeof(float), static_cast<int>(vertices.size()), attrs, 1},
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

#include "ExampleGame.h"

#include <array>
#include <cmath>
#include <vector>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/ShaderProgram.h"
#include "Render/Texture.h"
#include "Render/UniformBuffer.h"
#include "Render/UniformData.h"
#include "Resource/ModelLoader.h"

#include "Ecs/Systems/SimplePhysicSystem.h"
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/SkyboxComponent.h"
#include "Ecs/Components/TextComponent.h"
#include "Ecs/Systems/CameraInputSystem.h"
#include "Ecs/Systems/CameraMatrixSystem.h"
#include "Ecs/Systems/RenderSystem.h"
#include "Ecs/Systems/SkyboxSystem.h"
#include "Ecs/Systems/UITextSystem.h"
#include "Render/Font.h"

ExampleGame::ExampleGame() {}

ExampleGame::~ExampleGame() {}

void ExampleGame::OnCreate()
{
    Engine::OnCreate();

    audioSystem->SetMasterVolume(100.0f);
    audioSystem->SetFireVolume(100.0f);
    audioSystem->SetAmbientVolume(100.0f);

    audioSystem->PlayEvent("Play_Ambient", AudioSystem::AmbientId);

    auto uniformBuffer = renderEngine->CreateUniformBuffer({sizeof(UniformData)});

    auto shaderUnlit =
        renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Unlit.vert", "Assets/Shaders/SimpleShader_Unlit.frag"});
    shaderUnlit->SetUniformBufferSlot("UniformData", 0);

    auto shaderLit =
        renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Lit.vert", "Assets/Shaders/SimpleShader_Lit.frag"});
    shaderLit->SetUniformBufferSlot("UniformData", 0);

    auto shaderSkybox =
        renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Skybox.vert", "Assets/Shaders/SimpleShader_Skybox.frag"});
    shaderSkybox->SetUniformBufferSlot("UniformData", 0);

    auto shaderFire =
        renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Fire.vert", "Assets/Shaders/SimpleShader_Fire.frag"});
    shaderFire->SetUniformBufferSlot("UniformData", 0);

    auto shaderText =
        renderEngine->CreateShaderProgram({"Assets/Shaders/SimpleShader_Text.vert", "Assets/Shaders/SimpleShader_Text.frag"});

    systems = std::make_unique<EcsSystems>(world);
    systems->Add(std::make_unique<CameraInputSystem>(inputSystem.get()));
    systems->Add(std::make_unique<SimplePhysicSystem>());
    systems->Add(std::make_unique<CameraMatrixSystem>(renderEngine.get(), uniformBuffer, window.get()));
    systems->Add(std::make_unique<SkyboxSystem>(renderEngine.get()));
    systems->Add(std::make_unique<RenderSystem>(renderEngine.get(), uniformBuffer));
    systems->Add(std::make_unique<UITextSystem>(renderEngine.get(), shaderText, window.get()));
    systems->Init();

    CreateUiText();

    CreateSkybox(shaderSkybox);

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

    for (int i = 0; i < 10; i++)
    {
        auto cubeEntity = LoadModel("Assets/Models/cube.obj", shaderLit);
        world.AddComponent<RigidbodyComponent>(cubeEntity);
        auto& cubeTransform = world.GetComponent<TransformComponent>(cubeEntity);
        cubeTransform.position = {0.5f * (float)i, 50.0f + (float)i, 0};
        cubeTransform.rotation.SetRotationX(30.0f + 10.0f * (float)i);
        
        auto& shaderComp = world.GetComponent<ShaderComponent>(cubeEntity);
        shaderComp.shaderType = ShaderRenderType::Lit;
    }

    auto floorEntity = LoadModel("Assets/Models/cube.obj", shaderLit);
    auto& floorTransform = world.GetComponent<TransformComponent>(floorEntity);
    floorTransform.position = {0, -2.0f, 0};
    floorTransform.scale = {20.0f, 0.5f, 20.0f};
    auto& floorMat = world.GetComponent<MaterialComponent>(floorEntity);
    floorMat.diffuseTexture = Texture::LoadFromFile("Assets/Textures/Stone.jpg");
    floorMat.diffuseColor = {1.0f, 1.0f, 1.0f, 1.0f};
    auto& floorShaderComp = world.GetComponent<ShaderComponent>(floorEntity);
    floorShaderComp.shaderType = ShaderRenderType::Lit;

    const float platformTopY = floorTransform.position.y + floorTransform.scale.y * 0.5f;
    const float torchInset = floorTransform.scale.x * 0.5f - 1.0f;
    const Vector3 corners[4] = {
        {torchInset, platformTopY, torchInset},
        {-torchInset, platformTopY, torchInset},
        {torchInset, platformTopY, -torchInset},
        {-torchInset, platformTopY, -torchInset},
    };
    for (const auto& corner : corners)
        CreateTorch(corner, shaderLit, shaderFire);
}

Entity ExampleGame::LoadModel(const std::string& path, ShaderProgramPtr shader)
{
    ModelData modelData = ModelLoader::Load(path);

    std::vector<TexturePtr> textures;
    for (auto& mat : modelData.materials)
    {
        if (!mat.diffuseTexturePath.empty())
            textures.push_back(Texture::LoadFromFile(mat.diffuseTexturePath));
        else
            textures.push_back(nullptr);
    }

    VertexAttributes attrs[] = {
        {sizeof(Vector3) / sizeof(float)}, {sizeof(Vector2) / sizeof(float)}, {sizeof(Vector3) / sizeof(float)}};

    Entity result = 0;

    for (auto& meshData : modelData.meshes)
    {
        const auto vao =
            renderEngine->CreateVertexArrayObject({static_cast<void*>(meshData.vertices.data()), sizeof(MeshVertex),
                                                   static_cast<int>(meshData.vertices.size()), attrs, 3},
                                                  {static_cast<void*>(meshData.indices.data()),
                                                   static_cast<int>(meshData.indices.size() * sizeof(unsigned int))});

        const auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity);
        world.AddComponent<AABB>(entity);

        auto& mesh = world.AddComponent<MeshComponent>(entity);
        mesh.vao = vao;
        mesh.indexCount = static_cast<unsigned int>(meshData.indices.size());

        auto& shaderComp = world.AddComponent<ShaderComponent>(entity);
        shaderComp.shader = shader;

        if (meshData.materialIndex >= 0 && meshData.materialIndex < static_cast<int>(modelData.materials.size()))
        {
            auto& matComp = world.AddComponent<MaterialComponent>(entity);
            matComp.diffuseTexture = textures[meshData.materialIndex];
            matComp.diffuseColor = modelData.materials[meshData.materialIndex].diffuseColor;
        }

        if (result == 0)
            result = entity;
    }

    return result;
}

void ExampleGame::CreateSkybox(ShaderProgramPtr shader)
{
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

void ExampleGame::CreateTorch(const Vector3& basePosition, ShaderProgramPtr handleShader, ShaderProgramPtr fireShader)
{
    const float handleHeight = 1.2f;
    const float handleRadius = 0.12f;

    auto handleEntity = LoadModel("Assets/Models/cube.obj", handleShader);
    auto& handleTransform = world.GetComponent<TransformComponent>(handleEntity);
    handleTransform.position = basePosition + Vector3(0.0f, handleHeight * 0.5f, 0.0f);
    handleTransform.scale = {handleRadius, handleHeight, handleRadius};
    auto& handleMat = world.GetComponent<MaterialComponent>(handleEntity);
    handleMat.diffuseTexture = nullptr;
    handleMat.diffuseColor = {0.35f, 0.22f, 0.12f, 1.0f};
    auto& handleShaderComp = world.GetComponent<ShaderComponent>(handleEntity);
    handleShaderComp.shaderType = ShaderRenderType::Lit;

    unsigned int flameIndexCount = 0;
    const auto flameVao = CreateFlameMesh(flameIndexCount);

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
    flameLight.intensity = 4.0f;
    flameLight.position = flameTransform.position + Vector3(0.0f, flameTransform.scale.y * 0.3f, 0.0f);
    flameLight.range = 12.0f;

    audioSystem->RegisterGameObject(flameEntity, "Torch");
    audioSystem->SetPosition(flameEntity, flameTransform.position.x, flameTransform.position.y, flameTransform.position.z);
    audioSystem->PlayEvent("Play_Fire", flameEntity);
}

VertexArrayObjectPtr ExampleGame::CreateFlameMesh(unsigned int& outIndexCount)
{
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

void ExampleGame::CreateUiText()
{
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

void ExampleGame::OnUpdate(float deltaTime)
{
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

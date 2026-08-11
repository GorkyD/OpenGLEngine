#include "FpsScene.h"
#include <cmath>
#include <cstdlib>
#include <imgui.h>
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/FogComponent.h"
#include "Ecs/Components/ParticleEmitterComponent.h"
#include "Ecs/Components/PostProcessComponent.h"
#include "Ecs/Components/ProceduralSkyComponent.h"
#include "Ecs/Components/ProjectileComponent.h"
#include "Ecs/Components/TerrainComponent.h"
#include "Ecs/Systems/FpsCounterSystem.h"
#include "Ecs/Components/RigidbodyComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/MeshNameComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Components/WorldTransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Input/InputSystem.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/Texture.h"
#include "Ecs/Systems/HitFlashSystem.h"
#include "Physics/Physics.h"
#include "Resource/EntityFactory.h"
#include "Resource/TerrainGenerator.h"
#include "Window/Window.h"

void FpsScene::OnLoad(Engine& engine)
{
    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();
    const auto& shaders = engine.GetShaders();

    engine.GetWindow()->SetCursorLocked(true);

    hudSystem.Init(renderEngine, shaders.hud, engine.GetWindow());

    if (auto* fpsCounter = engine.GetFpsCounterSystem())
        fpsCounter->color = {1.0f, 1.0f, 1.0f, 1.0f};

    CreateProceduralSky(engine);
    CreateTerrain(engine);

    cameraEntity = world.CreateEntity();
    auto& camTransform = world.AddComponent<TransformComponent>(cameraEntity);
    camTransform.position = {0.0f, 1.6f, 0.0f};
    world.AddComponent<CameraComponent>(cameraEntity);
    world.AddComponent<NameComponent>(cameraEntity).name = "Player";

    auto& playerCollider = world.AddComponent<ColliderComponent>(cameraEntity);
    playerCollider.center = {0.0f, -0.6f, 0.0f};
    playerCollider.size = {0.6f, 1.8f, 0.6f};
    playerCollider.restitution = 0.0f;
    playerCollider.layer = PhysicsLayers::Player;

    characterController.Configure(world, cameraEntity);
    characterController.walkSpeed = moveSpeed;
    characterController.jumpSpeed = jumpSpeed;

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

    const auto flashlightEntity = world.CreateEntity();
    auto& flashlight = world.AddComponent<LightComponent>(flashlightEntity);
    flashlight.type = LightType::Spot;
    flashlight.color = {1.0f, 0.97f, 0.88f};
    flashlight.intensity = flashlightIntensity;
    flashlight.range = 25.0f;
    flashlight.innerConeAngleDeg = 40.0f;
    flashlight.outerConeAngleDeg = 55.0f;

    armsEntity = EntityFactory::CreateSkinnedModelEntity(world, renderEngine, "Assets/Models/fps_arms.glb", shaders.skinned);

    if (armsEntity != 0)
    {
        world.AddComponent<GroupComponent>(armsEntity).name = "player";
        auto& armsOffsetComp = world.AddComponent<TransformOffsetComponent>(armsEntity);
        armsOffsetComp.procedural = true;
        armsOffsetComp.position = armsOffsetDefault;
        armsOffsetComp.rotationDegrees = armsRotationDefault;

        if (world.HasComponent<MeshComponent>(armsEntity))
        {
            auto& armsMesh = world.GetComponent<MeshComponent>(armsEntity);
            armsMesh.castsShadow = false;
            armsMesh.renderQueue = RenderQueue::Overlay;
        }
    }

    if (armsEntity != 0 && world.HasComponent<MaterialComponent>(armsEntity))
    {
        auto& armsMat = world.GetComponent<MaterialComponent>(armsEntity);
        armsMat.diffuseColor = {0.82f, 0.62f, 0.49f, 1.0f};
        armsMat.roughness = 0.75f;
        armsMat.metallic = 0.0f;
    }

    if (armsEntity != 0 && world.HasComponent<AnimatorComponent>(armsEntity))
    {
        auto& animatorComp = world.GetComponent<AnimatorComponent>(armsEntity);
        if (animatorComp.state)
        {
            Animator::Play(*animatorComp.state, "Idle");

            if (engine.GetMode() == EngineMode::Editor)
                Animator::Stop(*animatorComp.state);
        }
    }

    weaponEntity = EntityFactory::CreateModelEntity(world, renderEngine, "Assets/Models/Weapons/pistol.fbx", shaders.lit, &weaponEntities);
    for (const auto entity : weaponEntities)
    {
        world.AddComponent<GroupComponent>(entity).name = "weapon";

        auto& parent = world.AddComponent<ParentComponent>(entity);
        parent.parent = armsEntity;
        parent.attachBone = "DEF-hand.R";

        auto& weaponOffsetComp = world.AddComponent<TransformOffsetComponent>(entity);
        weaponOffsetComp.procedural = true;
        weaponOffsetComp.position = weaponOffsetDefault;
        weaponOffsetComp.rotationDegrees = weaponRotationDefault;
        weaponOffsetComp.scale = weaponScaleDefault;
        slideParts.push_back(false);

        auto& weaponShaderComp = world.GetComponent<ShaderComponent>(entity);
        weaponShaderComp.shaderType = ShaderRenderType::Lit;

        if (world.HasComponent<MeshComponent>(entity))
        {
            auto& weaponMesh = world.GetComponent<MeshComponent>(entity);
            weaponMesh.castsShadow = false;
            weaponMesh.renderQueue = RenderQueue::Overlay;
        }

        if (world.HasComponent<MaterialComponent>(entity))
        {
            auto& weaponMat = world.GetComponent<MaterialComponent>(entity);
            weaponMat.diffuseColor = {0.15f, 0.16f, 0.18f, 1.0f};
            weaponMat.roughness = 0.35f;
            weaponMat.metallic = 0.15f;
        }
    }

    if (!weaponEntities.empty())
    {
        muzzleEntity = world.CreateEntity();
        world.AddComponent<NameComponent>(muzzleEntity).name = "Muzzle";
        world.AddComponent<TransformComponent>(muzzleEntity).position = {0.0f, 200.0f, 800.0f};

        auto& muzzleParent = world.AddComponent<ParentComponent>(muzzleEntity);
        muzzleParent.parent = weaponEntities.front();
    }
}

void FpsScene::CreateProceduralSky(Engine& engine)
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

void FpsScene::CreateTerrain(Engine& engine)
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

void FpsScene::OnUnload(Engine& engine)
{
    engine.GetWindow()->SetCursorLocked(false);
}

void FpsScene::UpdateLookAndMove(Engine& engine, float deltaTime)
{
    auto& world = engine.GetWorld();
    auto* input = engine.GetInputSystem();

    auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);

    const bool uiCapturingInput = ImGui::GetIO().WantCaptureMouse;
    if (!uiCapturingInput)
    {
        yaw += input->GetMouseDeltaX() * mouseSensitivity;
        pitch -= input->GetMouseDeltaY() * mouseSensitivity;
    }

    if (pitch < -maxPitch)
        pitch = -maxPitch;
    if (pitch > maxPitch)
        pitch = maxPitch;

    cam.forward.x = std::cos(pitch) * std::sin(yaw);
    cam.forward.y = std::sin(pitch);
    cam.forward.z = std::cos(pitch) * std::cos(yaw);
    cam.forward = Vector3::Normalize(cam.forward);

    const Vector3 worldUp(0, 1, 0);
    cam.right = Vector3::Normalize(Vector3::Cross(worldUp, cam.forward));
    cam.up = Vector3::Cross(cam.forward, cam.right);

    const bool flashlightKeyPressed = input->IsKeyDown(Key::L);
    if (flashlightKeyPressed && !flashlightKeyWasPressed)
        flashlightOn = !flashlightOn;
    flashlightKeyWasPressed = flashlightKeyPressed;

    for (auto& [entityId, lightComponent] : world.GetPool<LightComponent>())
    {
        if (lightComponent.type != LightType::Spot)
            continue;
        lightComponent.position = camTransform.position;
        lightComponent.direction = cam.forward;
        lightComponent.intensity = flashlightOn ? flashlightIntensity : 0.0f;
        break;
    }

    const Vector3 forwardOnGround = Vector3::Normalize({cam.forward.x, 0.0f, cam.forward.z});
    const Vector3 rightOnGround = Vector3::Normalize({cam.right.x, 0.0f, cam.right.z});

    Vector3 moveDir(0, 0, 0);
    if (input->IsKeyDown(Key::W))
        moveDir += forwardOnGround;
    if (input->IsKeyDown(Key::S))
        moveDir -= forwardOnGround;
    if (input->IsKeyDown(Key::D))
        moveDir += rightOnGround;
    if (input->IsKeyDown(Key::A))
        moveDir -= rightOnGround;

    const bool hasInput = Vector3::Dot(moveDir, moveDir) > 0.0001f;
    if (hasInput)
        moveDir = Vector3::Normalize(moveDir);

    const bool sprinting = input->IsKeyDown(Key::LShift);
    const bool wantsJump = input->IsKeyDown(Key::Space);

    characterController.Update(world, moveDir, wantsJump, sprinting, deltaTime);

    isMovingThisFrame = hasInput && characterController.IsGrounded();
}

void FpsScene::OnUpdate(Engine& engine, float deltaTime)
{
    auto& world = engine.GetWorld();
    auto* input = engine.GetInputSystem();

    if (cameraEntity == 0)
        return;

    hudSystem.Draw();

    UpdateLookAndMove(engine, deltaTime);
    UpdateProjectiles(world);
    UpdateMuzzleFlashes(world, deltaTime);

    const bool isMoving = isMovingThisFrame;

    if (slideRecoilTimer >= 0.0f)
    {
        slideRecoilTimer += deltaTime;
        if (slideRecoilTimer >= slideRecoilDuration)
            slideRecoilTimer = -1.0f;
    }

    if (armsEntity == 0)
        return;

    UpdateViewModel(world);

    if (!world.HasComponent<AnimatorComponent>(armsEntity))
        return;

    auto& animatorComp = world.GetComponent<AnimatorComponent>(armsEntity);
    if (!animatorComp.state)
        return;

    AnimatorState& state = *animatorComp.state;

    const bool firePressed = input->IsKeyDown(Key::F) || (input->IsLeftMouseDown() && !ImGui::GetIO().WantCaptureMouse);
    const bool reloadPressed = input->IsKeyDown(Key::R);

    if (inAction)
    {
        actionTimer += deltaTime;
        if (actionTimer >= actionDuration)
        {
            inAction = false;
            Animator::Play(state, isMoving ? "Walk" : "Idle");
        }
    }
    else if (firePressed && !firePressedLastFrame)
    {
        StartAction(state, "Fire");
        slideRecoilTimer = 0.0f;
        FireProjectile(engine);
    }
    else if (reloadPressed && !reloadPressedLastFrame)
    {
        StartAction(state, "ReloadMag");
    }
    else if (isMoving != wasMoving)
    {
        Animator::Play(state, isMoving ? "Walk" : "Idle");
    }

    wasMoving = isMoving;
    firePressedLastFrame = firePressed;
    reloadPressedLastFrame = reloadPressed;

    AttachWeaponToHand(world);
}

void FpsScene::UpdateViewModel(EcsWorld& world)
{
    if (cameraEntity == 0 || armsEntity == 0)
        return;

    auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);

    Matrix4 rotation;
    rotation.matrix[0][0] = cam.right.x;
    rotation.matrix[0][1] = cam.right.y;
    rotation.matrix[0][2] = cam.right.z;
    rotation.matrix[1][0] = cam.up.x;
    rotation.matrix[1][1] = cam.up.y;
    rotation.matrix[1][2] = cam.up.z;
    rotation.matrix[2][0] = -cam.forward.x;
    rotation.matrix[2][1] = -cam.forward.y;
    rotation.matrix[2][2] = -cam.forward.z;

    auto& armsOffsetComp = world.GetComponent<TransformOffsetComponent>(armsEntity);
    auto& armsTransform = world.GetComponent<TransformComponent>(armsEntity);
    armsTransform.rotation = armsOffsetComp.GetRotationMatrix() * rotation;
    armsTransform.scale = {armsScale.x * armsOffsetComp.scale.x, armsScale.y * armsOffsetComp.scale.y, armsScale.z * armsOffsetComp.scale.z};
    armsTransform.position = camTransform.position + cam.right * (armsOffset.x + armsOffsetComp.position.x) + cam.up * (armsOffset.y + armsOffsetComp.position.y) +
                             cam.forward * (armsOffset.z + armsOffsetComp.position.z);

    AttachWeaponToHand(world);
}

namespace
{
    float RandomRange(const float minValue, const float maxValue)
    {
        const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        return minValue + t * (maxValue - minValue);
    }
}

Vector3 FpsScene::ResolveAimPoint(EcsWorld& world, const Vector3& eyePosition, const Vector3& forward) const
{
    RaycastQuery query;
    query.origin = eyePosition;
    query.direction = forward;
    query.maxDistance = aimRayDistance;
    query.layerMask = PhysicsLayers::All & ~PhysicsLayers::Player;
    query.ignoreEntity = cameraEntity;

    RaycastHit hit;
    if (Physics::Raycast(world, query, hit))
        return hit.point;

    return eyePosition + forward * aimRayDistance;
}

Vector3 FpsScene::ResolveMuzzlePosition(EcsWorld& world, const Vector3& eyePosition, const Vector3& forward) const
{
    if (muzzleEntity == 0 || !world.HasComponent<WorldTransformComponent>(muzzleEntity))
        return eyePosition + forward * muzzleForwardOffset;

    const auto& matrix = world.GetComponent<WorldTransformComponent>(muzzleEntity).matrix;
    return {matrix.matrix[3][0], matrix.matrix[3][1], matrix.matrix[3][2]};
}

void FpsScene::FireProjectile(Engine& engine)
{
    auto& world = engine.GetWorld();

    const auto& cam = world.GetComponent<CameraComponent>(cameraEntity);
    const auto& camTransform = world.GetComponent<TransformComponent>(cameraEntity);

    const Vector3 aimPoint = ResolveAimPoint(world, camTransform.position, cam.forward);
    const Vector3 spawnPosition = ResolveMuzzlePosition(world, camTransform.position, cam.forward);

    Vector3 fireDirection = aimPoint - spawnPosition;
    if (fireDirection.LengthSquared() < 0.0001f)
        fireDirection = cam.forward;
    fireDirection = Vector3::Normalize(fireDirection);

    std::vector<Entity> created;
    if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), "Assets/Models/cube.obj", engine.GetShaders().lit, &created) == 0)
        return;

    for (const auto entity : created)
    {
        auto& transform = world.GetComponent<TransformComponent>(entity);
        transform.position = spawnPosition;
        transform.scale = {bulletRadius * 2.0f, bulletRadius * 2.0f, bulletRadius * 2.0f};

        if (world.HasComponent<MaterialComponent>(entity))
        {
            auto& material = world.GetComponent<MaterialComponent>(entity);
            material.diffuseTexture = nullptr;
            material.diffuseColor = {1.0f, 0.85f, 0.3f, 1.0f};
            material.emissiveColor = {1.0f, 0.75f, 0.2f};
            material.emissiveIntensity = 2.0f;
        }
    }

    auto& projectile = world.AddComponent<ProjectileComponent>(created.front());
    projectile.velocity = fireDirection * bulletSpeed;
    projectile.radius = bulletRadius;
    projectile.lifetime = bulletLifetime;
    projectile.owner = cameraEntity;
    projectile.layerMask = PhysicsLayers::All & ~PhysicsLayers::Player;

    auto& trail = world.AddComponent<ParticleEmitterComponent>(created.front());
    trail.color = {1.0f, 0.75f, 0.25f, 0.6f};
    trail.additive = true;
    trail.spawnRate = 90.0f;
    trail.velocityMin = {-0.05f, -0.05f, -0.05f};
    trail.velocityMax = {0.05f, 0.05f, 0.05f};
    trail.gravity = {0.0f, 0.0f, 0.0f};
    trail.lifetimeMin = 0.06f;
    trail.lifetimeMax = 0.12f;
    trail.startSize = bulletRadius * 5.0f;
    trail.endSize = 0.0f;
    trail.maxParticles = 60;

    projectileParts[created.front()] = created;

    SpawnMuzzleFlash(engine, spawnPosition, fireDirection);
}

void FpsScene::UpdateProjectiles(EcsWorld& world)
{
    finishedProjectiles.clear();

    for (auto& [entity, projectile] : world.GetPool<ProjectileComponent>())
    {
        if (!projectile.IsFinished())
            continue;

        if (projectile.hasHit && projectile.hitEntity != INVALID_ENTITY)
            HitFlashSystem::TriggerGroup(world, projectile.hitEntity);

        finishedProjectiles.push_back(entity);
    }

    for (const auto entity : finishedProjectiles)
    {
        const auto parts = projectileParts.find(entity);
        if (parts != projectileParts.end())
        {
            for (const auto part : parts->second)
                world.DestroyEntity(part);

            projectileParts.erase(parts);
            continue;
        }

        world.DestroyEntity(entity);
    }
}

void FpsScene::SpawnMuzzleFlash(Engine& engine, const Vector3& position, const Vector3& direction)
{
    auto& world = engine.GetWorld();

    const Entity entity = world.CreateEntity();
    world.AddComponent<TransformComponent>(entity).position = position;

    auto& emitter = world.AddComponent<ParticleEmitterComponent>(entity);
    emitter.color = {1.0f, 0.85f, 0.5f, 1.0f};
    emitter.additive = true;
    emitter.spawnRate = 0.0f;
    emitter.startSize = 0.3f;
    emitter.endSize = 0.05f;
    emitter.lifetimeMin = 0.035f;
    emitter.lifetimeMax = 0.06f;
    emitter.gravity = {0.0f, 0.0f, 0.0f};

    constexpr int flashParticleCount = 10;
    emitter.particles.reserve(flashParticleCount);

    for (int i = 0; i < flashParticleCount; i++)
    {
        const Vector3 jitter = {RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f)};

        Particle particle;
        particle.position = position;
        particle.velocity = direction * RandomRange(1.5f, 3.5f) + jitter * 1.2f;
        particle.lifetime = RandomRange(emitter.lifetimeMin, emitter.lifetimeMax);
        particle.age = 0.0f;
        emitter.particles.push_back(particle);
    }

    muzzleFlashes.push_back({entity, emitter.lifetimeMax});
}

void FpsScene::UpdateMuzzleFlashes(EcsWorld& world, float deltaTime)
{
    for (size_t i = 0; i < muzzleFlashes.size();)
    {
        muzzleFlashes[i].timeRemaining -= deltaTime;
        if (muzzleFlashes[i].timeRemaining <= 0.0f)
        {
            world.DestroyEntity(muzzleFlashes[i].entity);
            muzzleFlashes[i] = muzzleFlashes.back();
            muzzleFlashes.pop_back();
        }
        else
        {
            i++;
        }
    }
}

void FpsScene::OnEditorUpdate(Engine& engine, float deltaTime)
{
    if (armsEntity == 0)
        return;

    auto& world = engine.GetWorld();

    if (!editorStaticPlaced)
    {
        auto& armsOffsetComp = world.GetComponent<TransformOffsetComponent>(armsEntity);
        auto& armsTransform = world.GetComponent<TransformComponent>(armsEntity);
        armsTransform.position = editorStaticPosition;
        armsTransform.rotation = armsOffsetComp.GetRotationMatrix();
        armsTransform.scale = {armsScale.x * armsOffsetComp.scale.x, armsScale.y * armsOffsetComp.scale.y, armsScale.z * armsOffsetComp.scale.z};

        editorStaticPlaced = true;
    }

    AttachWeaponToHand(world);
}

void FpsScene::AttachWeaponToHand(EcsWorld& world)
{
    if (armsEntity == 0 || weaponEntity == 0)
        return;

    constexpr float deg2rad = 3.14159265f / 180.0f;

    Matrix4 center;
    center.SetTranslation(gripCenterOffset);
    Matrix4 pitch = Matrix4::FromAxisAngle({1.0f, 0.0f, 0.0f}, gripPitchDeg * deg2rad);
    Matrix4 roll = Matrix4::FromAxisAngle({0.0f, 0.0f, 1.0f}, gripRollDeg * deg2rad);
    Matrix4 scale;
    scale.SetScale({gripScale, gripScale, gripScale});
    const Matrix4 gripOffset = center * pitch * roll * scale;

    float recoilAmount = 0.0f;
    if (slideRecoilTimer >= 0.0f && slideRecoilDuration > 0.0001f)
    {
        const float t = slideRecoilTimer / slideRecoilDuration;
        recoilAmount = slideRecoilDistance * (t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f);
    }

    for (size_t i = 0; i < weaponEntities.size(); i++)
    {
        const Entity entity = weaponEntities[i];
        auto& offset = world.GetComponent<TransformOffsetComponent>(entity);

        Vector3 partPosition = offset.position;
        if (i < slideParts.size() && slideParts[i])
            partPosition += slideRecoilAxis * recoilAmount;

        Matrix4 offsetTranslation;
        offsetTranslation.SetTranslation(partPosition);

        Matrix4 partScale;
        partScale.SetScale(offset.scale);

        auto& localMatrix = world.HasComponent<LocalMatrixComponent>(entity) ? world.GetComponent<LocalMatrixComponent>(entity) : world.AddComponent<LocalMatrixComponent>(entity);
        localMatrix.matrix = partScale * offset.GetRotationMatrix() * gripOffset * offsetTranslation;
    }
}

void FpsScene::StartAction(AnimatorState& state, const std::string& clipName)
{
    inAction = true;
    actionTimer = 0.0f;
    actionDuration = GetClipDurationSeconds(state, clipName);
    Animator::Play(state, clipName, false);
}

float FpsScene::GetClipDurationSeconds(const AnimatorState& state, const std::string& clipName) const
{
    if (!state.clips)
        return 0.0f;

    for (const auto& clip : *state.clips)
    {
        if (clip.name != clipName)
            continue;

        const float ticksPerSecond = clip.ticksPerSecond > 0.0f ? clip.ticksPerSecond : 25.0f;
        return clip.durationTicks / ticksPerSecond;
    }

    return 0.0f;
}

void FpsScene::OnDebugUI(Engine& engine)
{
    for (auto& pair : engine.GetWorld().GetPool<PostProcessComponent>())
    {
        ImGui::TextUnformatted("Post FX");
        ImGui::DragFloat("Exposure", &pair.second.exposure, 0.01f, 0.1f, 4.0f);
        ImGui::DragFloat("Vignette Strength", &pair.second.vignetteStrength, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Vignette Radius", &pair.second.vignetteRadius, 0.01f, 0.0f, 1.5f);
        ImGui::DragFloat("Vignette Softness", &pair.second.vignetteSoftness, 0.01f, 0.0f, 1.5f);
        ImGui::Separator();
        break;
    }

    ImGui::TextUnformatted("Arms");
    ImGui::DragFloat3("Arms Scale", &armsScale.x, 0.01f);
    ImGui::DragFloat3("Arms Offset (R/U/F)", &armsOffset.x, 0.01f);

    ImGui::Separator();
    ImGui::TextUnformatted("Weapon Grip");
    ImGui::DragFloat3("Grip Center Offset", &gripCenterOffset.x, 0.01f);
    ImGui::DragFloat("Grip Pitch (deg)", &gripPitchDeg, 0.5f);
    ImGui::DragFloat("Grip Roll (deg)", &gripRollDeg, 0.5f);
    ImGui::DragFloat("Grip Scale", &gripScale, 0.001f, 0.0001f, 1.0f);

    ImGui::Separator();
    ImGui::TextUnformatted("Slide Recoil");
    ImGui::DragFloat3("Recoil Axis", &slideRecoilAxis.x, 0.01f);
    ImGui::DragFloat("Recoil Distance", &slideRecoilDistance, 0.001f);
    ImGui::DragFloat("Recoil Duration", &slideRecoilDuration, 0.005f, 0.01f, 2.0f);

    if (ImGui::Button("Test Recoil"))
        slideRecoilTimer = 0.0f;

    int markedParts = 0;
    for (size_t i = 0; i < slideParts.size(); i++)
        if (slideParts[i])
            markedParts++;

    ImGui::Text("timer: %.3f   marked parts: %d", slideRecoilTimer, markedParts);
    ImGui::Text("fire held: %s   in action: %s", firePressedLastFrame ? "yes" : "no", inAction ? "yes" : "no");

    auto& world = engine.GetWorld();
    ImGui::TextUnformatted("Slide Parts");
    for (size_t i = 0; i < weaponEntities.size() && i < slideParts.size(); i++)
    {
        const Entity entity = weaponEntities[i];

        std::string label;
        if (world.HasComponent<NameComponent>(entity))
            label = world.GetComponent<NameComponent>(entity).name;
        if (label.empty() && world.HasComponent<MeshNameComponent>(entity))
            label = world.GetComponent<MeshNameComponent>(entity).name;
        if (label.empty())
            label = "Part";

        label += " (" + std::to_string(entity) + ")";

        bool isSlide = slideParts[i];
        if (ImGui::Checkbox(label.c_str(), &isSlide))
            slideParts[i] = isSlide;
    }
}

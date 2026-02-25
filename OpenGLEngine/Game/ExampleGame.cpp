#include "ExampleGame.h"

#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Render/RenderEngine.h"
#include "Render/UniformBuffer.h"
#include "Render/UniformData.h"
#include "Render/ShaderProgram.h"
#include "Render/Texture.h"
#include "Resource/ModelLoader.h"

#include "Ecs/Systems/CameraInputSystem.h"
#include "Ecs/Systems/CameraMatrixSystem.h"
#include "Ecs/Systems/RenderSystem.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Systems/PhysicSystem.h"
#include "Ecs/Components/AABB.h"

ExampleGame::ExampleGame()
{
}

ExampleGame::~ExampleGame()
{
}

void ExampleGame::OnCreate()
{
	Engine::OnCreate();

	auto uniformBuffer = renderEngine->CreateUniformBuffer({sizeof(UniformData)});

	auto shader = renderEngine->CreateShaderProgram({
		"Assets/Shaders/SimpleShader.vert",
		"Assets/Shaders/SimpleShader.frag"
	});
	shader->SetUniformBufferSlot("UniformData", 0);

	systems = std::make_unique<EcsSystems>(world);
	systems->Add(std::make_unique<CameraInputSystem>(inputSystem.get()));
	systems->Add(std::make_unique<CameraMatrixSystem>(renderEngine.get(), uniformBuffer, window.get()));
	systems->Add(std::make_unique<RenderSystem>(renderEngine.get(), uniformBuffer));
	systems->Add(std::make_unique<PhysicSystem>());
	systems->Init();

	const auto cameraEntity = world.CreateEntity();
	auto& camTransform = world.AddComponent<TransformComponent>(cameraEntity);
	camTransform.position = {0, 1.0f, -3.0f};
	world.AddComponent<CameraComponent>(cameraEntity);
	world.AddComponent<FpsControllerComponent>(cameraEntity);

	for (int i = 0; i < 10; i++)
	{
		auto cubeEntity = LoadModel("Assets/Models/cube.obj", shader);
		world.AddComponent<RigidbodyComponent>(cubeEntity);
		auto& cubeTransform = world.GetComponent<TransformComponent>(cubeEntity);
		cubeTransform.position = { 0.5f * (float)i, 50.0f + (float)i, 0};
		cubeTransform.rotation.SetRotationX(30.0f + 10.0f * (float)i);
	}

	auto floorEntity = LoadModel("Assets/Models/cube.obj", shader);
	auto& floorTransform = world.GetComponent<TransformComponent>(floorEntity);
	floorTransform.position = { 0, -2.0f, 0 };
	floorTransform.scale = { 20.0f, 0.5f, 20.0f };
	auto& floorMat = world.GetComponent<MaterialComponent>(floorEntity);
	floorMat.diffuseTexture = nullptr;
	floorMat.diffuseColor = { 0.35f, 0.55f, 0.35f, 1.0f };
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
		{sizeof(Vector3) / sizeof(float)},
		{sizeof(Vector2) / sizeof(float)},
		{sizeof(Vector3) / sizeof(float)}
	};

	Entity result = 0;

	for (auto& meshData : modelData.meshes)
	{
		const auto vao = renderEngine->CreateVertexArrayObject(
			{
				static_cast<void*>(meshData.vertices.data()),
				sizeof(MeshVertex),
				static_cast<int>(meshData.vertices.size()),
				attrs,
				3
			},
			{
				static_cast<void*>(meshData.indices.data()),
				static_cast<int>(meshData.indices.size() * sizeof(unsigned int))
			});

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

		if (result == 0) result = entity;
	}

	return result;
}


void ExampleGame::OnUpdate(float deltaTime)
{
}

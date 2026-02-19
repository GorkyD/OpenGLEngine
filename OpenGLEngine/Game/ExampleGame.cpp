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
		L"Assets/Shaders/SimpleShader.vert",
		L"Assets/Shaders/SimpleShader.frag"
	});
	shader->SetUniformBufferSlot("UniformData", 0);

	systems = std::make_unique<EcsSystems>(world);
	systems->Add(std::make_unique<CameraInputSystem>(inputSystem.get()));
	systems->Add(std::make_unique<CameraMatrixSystem>(renderEngine.get(), uniformBuffer, window.get()));
	systems->Add(std::make_unique<RenderSystem>(renderEngine.get()));
	systems->Init();

	auto cameraEntity = world.CreateEntity();
	auto& camTransform = world.AddComponent<TransformComponent>(cameraEntity);
	camTransform.position = {0, 1.0f, -3.0f};
	world.AddComponent<CameraComponent>(cameraEntity);
	world.AddComponent<FpsControllerComponent>(cameraEntity);

	LoadModel("Assets/Models/cube.obj", shader);
}

void ExampleGame::LoadModel(const std::string& path, ShaderProgramPtr shader)
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

	for (auto& meshData : modelData.meshes)
	{
		auto vao = renderEngine->CreateVertexArrayObject(
			{
				(void*)meshData.vertices.data(),
				sizeof(MeshVertex),
				(int)meshData.vertices.size(),
				attrs,
				3
			},
			{
				(void*)meshData.indices.data(),
				(int)(meshData.indices.size() * sizeof(unsigned int))
			});

		auto entity = world.CreateEntity();
		world.AddComponent<TransformComponent>(entity);

		auto& mesh = world.AddComponent<MeshComponent>(entity);
		mesh.vao = vao;
		mesh.indexCount = (unsigned int)meshData.indices.size();

		auto& shaderComp = world.AddComponent<ShaderComponent>(entity);
		shaderComp.shader = shader;

		if (meshData.materialIndex >= 0 && meshData.materialIndex < (int)modelData.materials.size())
		{
			auto& matComp = world.AddComponent<MaterialComponent>(entity);
			matComp.diffuseTexture = textures[meshData.materialIndex];
			matComp.diffuseColor = modelData.materials[meshData.materialIndex].diffuseColor;
		}
	}
}

void ExampleGame::OnUpdate(float deltaTime)
{
}

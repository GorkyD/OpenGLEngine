#include "ExampleGame.h"

#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Render/RenderEngine.h"
#include "Render/UniformBuffer.h"
#include "Render/UniformData.h"
#include "Render/ShaderProgram.h"

#include "Ecs/Systems/CameraInputSystem.h"
#include "Ecs/Systems/CameraMatrixSystem.h"
#include "Ecs/Systems/RenderSystem.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"

struct Vertex
{
	Vector3 position;
	Vector2 textureCoordinates;
};

ExampleGame::ExampleGame()
{
}

ExampleGame::~ExampleGame()
{
}

void ExampleGame::OnCreate()
{
	Engine::OnCreate();

	const Vector3 positionsList[] =
	{
		Vector3(-0.5f,-0.5f,-0.5f),
		Vector3(-0.5f,0.5f,-0.5f),
		Vector3(0.5f,0.5f,-0.5f),
		Vector3(0.5f,-0.5f,-0.5f),

		Vector3(0.5f,-0.5f,0.5f),
		Vector3(0.5f,0.5f,0.5f),
		Vector3(-0.5f,0.5f,0.5f),
		Vector3(-0.5f,-0.5f,0.5f)
	};

	const Vector2 textureCoordinatesList[] =
	{
		Vector2(0,0),
		Vector2(0,1),
		Vector2(1,0),
		Vector2(1,1)
	};

	const Vertex verticesList[] =
	{
		{ positionsList[0],textureCoordinatesList[1] },
		{ positionsList[1],textureCoordinatesList[0] },
		{ positionsList[2],textureCoordinatesList[2] },
		{ positionsList[3],textureCoordinatesList[3] },

		{ positionsList[4],textureCoordinatesList[1] },
		{ positionsList[5],textureCoordinatesList[0] },
		{ positionsList[6],textureCoordinatesList[2] },
		{ positionsList[7],textureCoordinatesList[3] },

		{ positionsList[1],textureCoordinatesList[1] },
		{ positionsList[6],textureCoordinatesList[0] },
		{ positionsList[5],textureCoordinatesList[2] },
		{ positionsList[2],textureCoordinatesList[3] },

		{ positionsList[7],textureCoordinatesList[1] },
		{ positionsList[0],textureCoordinatesList[0] },
		{ positionsList[3],textureCoordinatesList[2] },
		{ positionsList[4],textureCoordinatesList[3] },

		{ positionsList[3],textureCoordinatesList[1] },
		{ positionsList[2],textureCoordinatesList[0] },
		{ positionsList[5],textureCoordinatesList[2] },
		{ positionsList[4],textureCoordinatesList[3] },

		{ positionsList[7],textureCoordinatesList[1] },
		{ positionsList[6],textureCoordinatesList[0] },
		{ positionsList[1],textureCoordinatesList[2] },
		{ positionsList[0],textureCoordinatesList[3] }
	};

	const unsigned int indicesList[] =
	{
		0,1,2,  2,3,0,
		4,5,6,  6,7,4,
		8,9,10, 10,11,8,
		12,13,14, 14,15,12,
		16,17,18, 18,19,16,
		20,21,22, 22,23,20
	};

	VertexAttributes attributesList[] =
	{
		sizeof(Vector3)/sizeof(float),
		sizeof(Vector2)/sizeof(float)
	};

	auto vao = renderEngine->CreateVertexArrayObject(
		{
			(void*)verticesList,
			sizeof(Vertex),
			sizeof(verticesList)/sizeof(Vertex),
			attributesList,
			sizeof(attributesList)/sizeof(VertexAttributes)
		},{
			(void*)indicesList,
			sizeof(indicesList)
		});

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
	camTransform.position = {0, 0.5f, -2.5f};
	world.AddComponent<CameraComponent>(cameraEntity);
	world.AddComponent<FpsControllerComponent>(cameraEntity);

	auto cubeEntity = world.CreateEntity();
	world.AddComponent<TransformComponent>(cubeEntity);
	auto& mesh = world.AddComponent<MeshComponent>(cubeEntity);
	mesh.vao = vao;
	mesh.indexCount = 36;
	auto& shaderComp = world.AddComponent<ShaderComponent>(cubeEntity);
	shaderComp.shader = shader;
}

void ExampleGame::OnUpdate(float deltaTime)
{
}

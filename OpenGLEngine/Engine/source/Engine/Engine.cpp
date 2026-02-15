#include <chrono>
#include "Window/Window.h"
#include "Engine/Engine.h"

#include "Entity/EntitySystem.h"
#include "Input/InputSystem.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Render/RenderEngine.h"
#include "Render/UniformBuffer.h"
#include "Render/ShaderProgram.h"

struct UniformData
{
	Matrix4 world;
	Matrix4 view;
	Matrix4 projection;
};

struct Vertex
{
	Vector3 position;
	Vector2 textureCoordinates;
};


Engine::Engine()
{
	renderEngine = std::make_unique<RenderEngine>();
	window = std::make_unique<Window>();
	inputSystem = std::make_shared<InputSystem>(static_cast<HWND>(window->GetWindowInstance()));
	entitySystem = std::make_unique<EntitySystem>();

	window->MakeCurrentContext();
	renderEngine->SetViewPort(window->GetInnerSize());
}

Engine::~Engine(){}

void Engine::OnCreate()
{
	Vector3 positionsList[] =
	{
		//front face
		Vector3(-0.5f,-0.5f,-0.5f),
		Vector3(-0.5f,0.5f,-0.5f),
		Vector3(0.5f,0.5f,-0.5f),
		Vector3(0.5f,-0.5f,-0.5f),

		//back face
		Vector3(0.5f,-0.5f,0.5f),
		Vector3(0.5f,0.5f,0.5f),
		Vector3(-0.5f,0.5f,0.5f),
		Vector3(-0.5f,-0.5f,0.5f)
	};

	Vector2 textureCoordinatesList[] =
	{
		Vector2(0,0),
		Vector2(0,1),
		Vector2(1,0),
		Vector2(1,1)
	};

	Vertex verticesList[] =
	{
		//front face
		{ positionsList[0],textureCoordinatesList[1] },
		{ positionsList[1],textureCoordinatesList[0] },
		{ positionsList[2],textureCoordinatesList[2] },
		{ positionsList[3],textureCoordinatesList[3] },

		//back face
		{ positionsList[4],textureCoordinatesList[1] },
		{ positionsList[5],textureCoordinatesList[0] },
		{ positionsList[6],textureCoordinatesList[2] },
		{ positionsList[7],textureCoordinatesList[3] },

		//top face
		{ positionsList[1],textureCoordinatesList[1] },
		{ positionsList[6],textureCoordinatesList[0] },
		{ positionsList[5],textureCoordinatesList[2] },
		{ positionsList[2],textureCoordinatesList[3] },

		//bottom face
		{ positionsList[7],textureCoordinatesList[1] },
		{ positionsList[0],textureCoordinatesList[0] },
		{ positionsList[3],textureCoordinatesList[2] },
		{ positionsList[4],textureCoordinatesList[3] },

		//right face
		{ positionsList[3],textureCoordinatesList[1] },
		{ positionsList[2],textureCoordinatesList[0] },
		{ positionsList[5],textureCoordinatesList[2] },
		{ positionsList[4],textureCoordinatesList[3] },

		//left face
		{ positionsList[7],textureCoordinatesList[1] },
		{ positionsList[6],textureCoordinatesList[0] },
		{ positionsList[1],textureCoordinatesList[2] },
		{ positionsList[0],textureCoordinatesList[3] }
	};

	unsigned int indicesList[] =
	{
		//front
		0,1,2,
		2,3,0,

		//back
		4,5,6,
		6,7,4,

		//top
		8,9,10,
		10,11,8,

		//bottom
		12,13,14,
		14,15,12,

		//right
		16,17,18,
		18,19,16,

		//left
		20,21,22,
		22,23,20
	};


	VertexAttributes attributesList[] = 
	{
		sizeof(Vector3)/sizeof(float),
		sizeof(Vector2) / sizeof(float)
	};

	polygonVaoPtr = renderEngine->CreateVertexArrayObject(
		{
			(void*)verticesList,
			sizeof(Vertex),
			sizeof(verticesList)/sizeof(Vertex),

			attributesList,
			sizeof(attributesList)/sizeof(VertexAttributes)
		},{
			(void*) indicesList,
			sizeof(indicesList)
		});

	uniformBufferPtr = renderEngine->CreateUniformBuffer({
		sizeof(UniformData)
		});

	shaderProgramPtr = renderEngine->CreateShaderProgram(
{
	L"Assets/Shaders/SimpleShader.vert",
	L"Assets/Shaders/SimpleShader.frag"
		});

	shaderProgramPtr->SetUniformBufferSlot("UniformData", 0);

	camera.SetPosition(Vector3(0, 0.5f, -2.5f));
}

void Engine::OnUpdateInternal()
{
	auto currentTime = std::chrono::system_clock::now();
	auto elapsedSeconds = std::chrono::duration<double>();
	if (previousTime.time_since_epoch().count())
		elapsedSeconds = currentTime - previousTime;
	previousTime = currentTime;

	const auto deltaTime = static_cast<float>(elapsedSeconds.count());

	cameraMovement.Update(camera, *inputSystem, deltaTime);
	inputSystem->Update();

	OnUpdate(deltaTime);
	entitySystem->Update(deltaTime);


	scale += speed * deltaTime;

	Matrix4 world, projection, temp;

	temp.SetScale(Vector3(1, 1, 1));
	world *= temp;

	temp.SetIdentity();
	temp.SetRotationZ(scale);
	world *= temp;

	temp.SetIdentity();
	temp.SetRotationY(scale);
	world *= temp;

	temp.SetIdentity();
	temp.SetRotationX(scale);
	world *= temp;

	temp.SetIdentity();
	temp.SetTranslation(Vector3(0, 0, 0));
	world *= temp;

	const auto displaySize = window->GetInnerSize();
	const float nearPlane = 0.01f;
	const float farPlane = 100.0f;
	const float fovY = 0.7854f;
	const float aspect = static_cast<float>(displaySize.width) / static_cast<float>(displaySize.height);

	projection.SetPerspectiveLeftHanded(fovY, aspect, nearPlane, farPlane);

	UniformData data = { world, camera.GetViewMatrix(), projection };
	uniformBufferPtr->SetData(&data);



	renderEngine->Clear(Vector4(0, 0, 0, 1));

	renderEngine->SetFaceCulling(CullingType::BackFace);
	renderEngine->SetWindingOrder(ClockWise);
	renderEngine->SetVertexArrayObject(polygonVaoPtr);
	renderEngine->SetUniformBuffer(uniformBufferPtr, 0);
	renderEngine->SetShaderProgram(shaderProgramPtr);
	//renderEngine->DrawTriangles(Strip, polygonVaoPtr->GetVertexBufferSize(), 0);
	renderEngine->DrawIndexedTriangles(List, 36);


	window->Present(false);
}

void Engine::OnQuit()
{
}


void Engine::Quit()
{
	is_Running = false;
}

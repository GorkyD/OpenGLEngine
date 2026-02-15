#include <chrono>
#include "Window/OWindow.h"
#include "Engine/OEngine.h"

#include "Entity/OEntitySystem.h"
#include "Input/InputSystem.h"
#include "Math/OMath4.h"
#include "Math/OVector3.h"
#include "Math/OVector2.h"
#include "Math/OVector4.h"
#include "Render/ORenderEngine.h"
#include "Render/OUniformBuffer.h"
#include "Render/OShaderProgram.h"

struct UniformData
{
	OMath4 world;
	OMath4 projection;
};

struct Vertex
{
	OVector3 position;
	OVector2 textureCoordinates;
};


OEngine::OEngine()
{
	renderEngine = std::make_unique<ORenderEngine>();
	window = std::make_unique<OWindow>();
	inputSystem = std::make_shared<InputSystem>(static_cast<HWND>(window->GetWindowInstance()));
	entitySystem = std::make_unique<OEntitySystem>();

	window->MakeCurrentContext();
	renderEngine->SetViewPort(window->GetInnerSize());
}

OEngine::~OEngine(){}

void OEngine::OnCreate()
{
	OVector3 positionsList[] =
	{
		//front face
		OVector3(-0.5f,-0.5f,-0.5f),
		OVector3(-0.5f,0.5f,-0.5f),
		OVector3(0.5f,0.5f,-0.5f),
		OVector3(0.5f,-0.5f,-0.5f),

		//back face
		OVector3(0.5f,-0.5f,0.5f),
		OVector3(0.5f,0.5f,0.5f),
		OVector3(-0.5f,0.5f,0.5f),
		OVector3(-0.5f,-0.5f,0.5f)
	};

	OVector2 textureCoordinatesList[] =
	{
		OVector2(0,0),
		OVector2(0,1),
		OVector2(1,0),
		OVector2(1,1)
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


	OVertexAttributes attributesList[] = 
	{
		sizeof(OVector3)/sizeof(float), //position
		sizeof(OVector2) / sizeof(float)  //texcoord
	};

	polygonVaoPtr = renderEngine->CreateVertexArrayObject(
		{
			(void*)verticesList,
			sizeof(Vertex),
			sizeof(verticesList)/sizeof(Vertex),

			attributesList,
			sizeof(attributesList)/sizeof(OVertexAttributes)
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
}

void OEngine::OnUpdateInternal()
{
	auto currentTime = std::chrono::system_clock::now();
	auto elapsedSeconds = std::chrono::duration<double>();
	if (previousTime.time_since_epoch().count())
		elapsedSeconds = currentTime - previousTime;
	previousTime = currentTime;

	const auto deltaTime = static_cast<float>(elapsedSeconds.count());

	OnUpdate(deltaTime);
	entitySystem->Update(deltaTime);


	scale += speed * deltaTime;
	auto currentScale = abs(sin(scale));

	OMath4 world,projection,temp;

	temp.SetScale(OVector3(1, 1, 1));
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
	temp.SetTranslation(OVector3(0, 0, 0));
	world *= temp;

	const auto displaySize = window->GetInnerSize();
	const float zoomFactor = 0.004f;
	const float nearPlane = 0.01f;
	const float farPlane = 100.0f;

	projection.SetOrthogonalLeftHanded(displaySize.width * zoomFactor, displaySize.height * zoomFactor, nearPlane, farPlane);

	UniformData data = { world, projection };
	uniformBufferPtr->SetData(&data);



	renderEngine->Clear(OVector4(0, 0, 0, 1));

	renderEngine->SetFaceCulling(OCullingType::BackFace);
	renderEngine->SetWindingOrder(ClockWise);
	renderEngine->SetVertexArrayObject(polygonVaoPtr);
	renderEngine->SetUniformBuffer(uniformBufferPtr, 0);
	renderEngine->SetShaderProgram(shaderProgramPtr);
	//renderEngine->DrawTriangles(Strip, polygonVaoPtr->GetVertexBufferSize(), 0);
	renderEngine->DrawIndexedTriangles(List, 36);


	window->Present(false);
}

void OEngine::OnQuit()
{
}


void OEngine::Quit()
{
	is_Running = false;
}

#pragma once
#include <memory>
#include <sstream>
#include <iostream>
#include <stdexcept>

class OVertexArrayObject;
class OUniformBuffer;
class OShaderProgram;

typedef std::shared_ptr<OVertexArrayObject> OVertexArrayObjectPtr;
typedef std::shared_ptr<OUniformBuffer> OUniformBufferPtr;
typedef std::shared_ptr<OShaderProgram> OShaderProgramPtr;

struct OVertexAttributes
{
	int numElements = 0;
};

struct OVertexBufferDesc
{
	void* verticesList = nullptr;
	int vertexSize = 0;
	int listSize = 0;

	OVertexAttributes* attributesList = nullptr;
	int attributesListSize = 0;
};

struct OIndexBufferDesc
{
	void* indicesList = nullptr;
	int listSize = 0;
};

struct OShaderProgramDesc
{
	const wchar_t* vertexShaderFilePath;
	const wchar_t* fragmentShaderFilePath;
};

struct OUniformBufferDesc
{
	unsigned int size = 0;
};

enum OShaderType
{
	VertexType,
	FragmentType
};

enum OTriangleType
{
	List,
	Strip
};

enum OCullingType
{
	BackFace,
	FrontFace,
	Both
};

enum OWindingOrder
{
	ClockWise,
	CounterClockWise
};

#define OGL_ERROR(message)\
{\
std::stringstream m;\
m << "OGL_ERROR: " << (message) << std::endl;\
throw std::runtime_error(m.str());\
}

#define OGL_WARNING(message)\
std::wclog << "OGL_WARNING: " << message << std::endl;


#define OGL_INFO(message)\
std::wclog << "OGL_INFO: " << message << std::endl;
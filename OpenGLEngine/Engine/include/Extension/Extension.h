#pragma once
#include <memory>
#include <sstream>
#include <iostream>
#include <stdexcept>

class VertexArrayObject;
class UniformBuffer;
class ShaderProgram;
class Texture;

typedef std::shared_ptr<VertexArrayObject> VertexArrayObjectPtr;
typedef std::shared_ptr<UniformBuffer> UniformBufferPtr;
typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;
typedef std::shared_ptr<Texture> TexturePtr;

struct VertexAttributes
{
	int numElements = 0;
};

struct VertexBufferDesc
{
	void* verticesList = nullptr;
	int vertexSize = 0;
	int listSize = 0;

	VertexAttributes* attributesList = nullptr;
	int attributesListSize = 0;
};

struct IndexBufferDesc
{
	void* indicesList = nullptr;
	int listSize = 0;
};

struct ShaderProgramDesc
{
	const char* vertexShaderFilePath;
	const char* fragmentShaderFilePath;
};

struct UniformBufferDesc
{
	unsigned int size = 0;
};

enum ShaderType
{
	VertexType,
	FragmentType
};

enum TriangleType
{
	List,
	Strip
};

enum CullingType
{
	BackFace,
	FrontFace,
	Both
};

enum WindingOrder
{
	ClockWise,
	CounterClockWise
};

enum class Key
{
	Unknown = 0,

	A, B, C, D, E, F, G,
	H, I, J, K, L, M, N,
	O, P, Q, R, S, T, U,
	V, W, X, Y, Z,

	Escape,
	Space,
	Enter,

	Left,
	Right,
	Up,
	Down,

	LShift,
	RShift,
	LControl,
	RControl,
	LAlt,
	RAlt,

	Count
};

#define OGL_ERROR(message)\
{\
std::stringstream m;\
m << "OGL_ERROR: " << (message) << std::endl;\
throw std::runtime_error(m.str());\
}

#define OGL_WARNING(message)\
std::clog << "OGL_WARNING: " << message << std::endl;


#define OGL_INFO(message)\
std::clog << "OGL_INFO: " << message << std::endl;

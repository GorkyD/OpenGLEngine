#include "Render/ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <Windows.h>
#include <glad/glad.h>

ShaderProgram::ShaderProgram(const ShaderProgramDesc& desc)
{
	programId = glCreateProgram();
	Attach(desc.vertexShaderFilePath, VertexType);
	Attach(desc.fragmentShaderFilePath, FragmentType);
	Link();
}

ShaderProgram::~ShaderProgram()
{
	for (const int attachedShader : attachedShaders)
	{
		glDetachShader(programId, attachedShader);
		glDeleteShader(attachedShader);
	}

	glDeleteProgram(programId);
}

void ShaderProgram::SetUniformBufferSlot(const char* name, unsigned int slot)
{
	unsigned int index = glGetUniformBlockIndex(programId, name);
	glUniformBlockBinding(programId, index, slot);
}


void ShaderProgram::Attach(const wchar_t* shaderFilePath,const ShaderType& type)
{
	std::ifstream shaderStream(shaderFilePath);
	std::string shaderCode;
	if (shaderStream.is_open())
	{
		std::stringstream sstr;
		sstr << shaderStream.rdbuf();
		shaderCode = sstr.str();
		shaderStream.close();
	}
	else
	{
		OGL_WARNING("ShaderProgram | " << shaderFilePath << " not found");
		return;
	}

	int shaderId = 0;
	if (type == VertexType)
		shaderId = glCreateShader(GL_VERTEX_SHADER);
	else if (type == FragmentType)
		shaderId = glCreateShader(GL_FRAGMENT_SHADER);

	const auto sourcePointer = shaderCode.c_str();
	glShaderSource(shaderId, 1, &sourcePointer, nullptr);
	glCompileShader(shaderId);

	int logLength = 0;
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> errorMessage(logLength + 1);
		glGetShaderInfoLog(shaderId, logLength, nullptr, errorMessage.data());
		OGL_WARNING("ShaderProgram | " << shaderFilePath << " compiled with errors: " <<std::endl << errorMessage.data())
		return;
	}

	glAttachShader(programId, shaderId);
	attachedShaders[(unsigned int)type] = shaderId;

	OGL_INFO("ShaderProgram | " << shaderFilePath << " compiled successfully");
}

void ShaderProgram::Link() const
{
	glLinkProgram(programId);

	int logLength = 0;
	glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
	if (logLength > 0)
	{
		std::vector<char> errorMessage(logLength + 1);
		glGetProgramInfoLog(programId, logLength, nullptr, errorMessage.data());
		OGL_WARNING("ShaderProgram | " << errorMessage.data())
		return;
	}
}

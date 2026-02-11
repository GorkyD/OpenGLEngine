#pragma once
#include "Extension/OExtension.h"

class OShaderProgram
{
	public:
		OShaderProgram(const OShaderProgramDesc& program);
		~OShaderProgram();

		int GetId() const { return programId;}
		void SetUniformBufferSlot(const char* name, unsigned int slot);
	private:
		void Attach(const wchar_t* shaderFilePath, const OShaderType& type);
		void Link() const;

		int programId = 0;
		int attachedShaders[2] = {};
};


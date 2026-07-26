#pragma once

#include "Extension/Extension.h"
#include <string>
#include <unordered_map>

class ShaderProgram
{
public:
    ShaderProgram(const ShaderProgramDesc& program);
    ~ShaderProgram();

    int GetId() const
    {
        return programId;
    }
    void SetUniformBufferSlot(const char* name, unsigned int slot);
    int GetUniformLocation(const char* name);

private:
    void Attach(const char* shaderFilePath, const ShaderType& type);
    void Link() const;

    int programId = 0;
    int attachedShaders[2] = {};
    std::unordered_map<std::string, int> uniformLocationCache;
};

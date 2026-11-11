#include "Render/ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>
#include <glad/glad.h>

ShaderProgram::ShaderProgram(const ShaderProgramDesc& desc)
{
    programId = glCreateProgram();
    Attach(desc.vertexShaderFilePath, VertexType);
    Attach(desc.fragmentShaderFilePath, FragmentType);
    Link(desc);
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

int ShaderProgram::GetUniformLocation(const char* name)
{
    const auto it = uniformLocationCache.find(std::string_view(name));
    if (it != uniformLocationCache.end())
        return it->second;

    const int location = glGetUniformLocation(programId, name);
    uniformLocationCache.emplace(name, location);
    return location;
}

void ShaderProgram::Attach(const char* shaderFilePath, const ShaderType& type)
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

    int compileStatus = GL_FALSE;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE)
    {
        int logLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            std::vector<char> errorMessage(logLength + 1);
            glGetShaderInfoLog(shaderId, logLength, nullptr, errorMessage.data());
            OGL_WARNING("ShaderProgram | " << shaderFilePath << " compiled with errors: " << std::endl << errorMessage.data())
        }

        glDeleteShader(shaderId);
        return;
    }

    glAttachShader(programId, shaderId);
    attachedShaders[(unsigned int)type] = shaderId;

    OGL_INFO("ShaderProgram | " << shaderFilePath << " compiled successfully");
}

void ShaderProgram::Link(const ShaderProgramDesc& desc) const
{
    glLinkProgram(programId);

    int linkStatus = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

    int logLength = 0;
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> log(logLength > 0 ? logLength + 1 : 1, '\0');
    if (logLength > 0)
        glGetProgramInfoLog(programId, logLength, nullptr, log.data());

    if (linkStatus == GL_FALSE)
        OGL_WARNING("ShaderProgram | link failed (" << desc.vertexShaderFilePath << ", " << desc.fragmentShaderFilePath << "): " << log.data())
    else if (logLength > 1)
        OGL_INFO("ShaderProgram | link log (" << desc.vertexShaderFilePath << ", " << desc.fragmentShaderFilePath << "): " << log.data())
}

#include "Render/OUniformBuffer.h"

#include <glad/glad.h>

OUniformBuffer::OUniformBuffer(const OUniformBufferDesc& desc)
{
	glGenBuffers(1, &id);
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBufferData(GL_UNIFORM_BUFFER, desc.size, nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	size = desc.size;

}

OUniformBuffer::~OUniformBuffer()
{
	glDeleteBuffers(1, &id);
}

void OUniformBuffer::SetData(void* data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

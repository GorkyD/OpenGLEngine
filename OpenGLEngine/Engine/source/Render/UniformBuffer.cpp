#include "Render/UniformBuffer.h"

#include <glad/glad.h>

UniformBuffer::UniformBuffer(const UniformBufferDesc& desc)
{
	glGenBuffers(1, &id);
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBufferData(GL_UNIFORM_BUFFER, desc.size, nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	size = desc.size;

}

UniformBuffer::~UniformBuffer()
{
	glDeleteBuffers(1, &id);
}

void UniformBuffer::SetData(void* data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBuffer::SetSubData(void* data, unsigned int offset, unsigned int subSize)
{
	glBindBuffer(GL_UNIFORM_BUFFER, id);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, subSize, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

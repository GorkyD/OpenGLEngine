#include "Render/RenderEngine.h"
#include <cassert>
#include <Windows.h>
#include <glad/glad.h>
#include "Render/ShaderProgram.h"
#include "Render/UniformBuffer.h"
#include "Render/VertexArrayObject.h"

void RenderEngine::Clear(const Vector4& color)
{
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderEngine::SetFaceCulling(const CullingType& type)
{
	auto cullType = GL_BACK;

	if (type == CullingType::FrontFace) cullType = GL_FRONT;
	else if (type == CullingType::BackFace) cullType = GL_BACK;
	else if (type == CullingType::Both) cullType = GL_FRONT_AND_BACK;

	glEnable(GL_CULL_FACE);
	glCullFace(cullType);
	glEnable(GL_DEPTH_TEST);
}

void RenderEngine::SetWindingOrder(const WindingOrder& order)
{
	auto orderType = GL_CW;

	if (order == ClockWise) orderType = GL_CW;
	else if (order == CounterClockWise) orderType = GL_CCW;

	glFrontFace(orderType);
}

void RenderEngine::SetViewPort(const Rect& rect)
{
	glViewport(rect.left, rect.top, rect.width, rect.height);
}

void RenderEngine::SetVertexArrayObject(const VertexArrayObjectPtr& vao)
{
	glBindVertexArray(vao->GetId());
}

void RenderEngine::SetShaderProgram(const ShaderProgramPtr& program)
{
	glUseProgram(program->GetId());
}

void RenderEngine::SetUniformBuffer(const UniformBufferPtr& buffer, unsigned int slot)
{
	glBindBufferBase(GL_UNIFORM_BUFFER,slot, buffer->GetId());
}

void RenderEngine::DrawTriangles(const TriangleType& type, unsigned int vertexCount,unsigned int offset)
{
	auto glTriType = GL_TRIANGLES;

	switch (type)
	{
		case TriangleType::List:{ glTriType = GL_TRIANGLES; break; }
		case TriangleType::Strip: { glTriType = GL_TRIANGLE_STRIP; break; }
	}

	glDrawArrays(glTriType, offset, vertexCount);
}

void RenderEngine::DrawIndexedTriangles(const TriangleType& type, unsigned int indicesCount)
{
	auto glTriType = GL_TRIANGLES;

	switch (type)
	{
	case List: { glTriType = GL_TRIANGLES; break; }
	case Strip: { glTriType = GL_TRIANGLE_STRIP; break; }
	}

	glDrawElements(glTriType, indicesCount, GL_UNSIGNED_INT, nullptr);
}

VertexArrayObjectPtr RenderEngine::CreateVertexArrayObject(const VertexBufferDesc& desc)
{
	return std::make_shared<VertexArrayObject>(desc);
}

VertexArrayObjectPtr RenderEngine::CreateVertexArrayObject(const VertexBufferDesc& vertexBufferDesc, const IndexBufferDesc& indexBufferDesc)
{
	return std::make_shared<VertexArrayObject>(vertexBufferDesc, indexBufferDesc);
}

ShaderProgramPtr RenderEngine::CreateShaderProgram(const ShaderProgramDesc& desc)
{
	return std::make_shared<ShaderProgram>(desc);
}

UniformBufferPtr RenderEngine::CreateUniformBuffer(const UniformBufferDesc& desc)
{
	return std::make_shared<UniformBuffer>(desc);
}

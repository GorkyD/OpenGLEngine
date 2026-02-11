#include "Render/ORenderEngine.h"
#include <cassert>
#include <Windows.h>
#include <glad/glad.h>
#include "Render/OShaderProgram.h"
#include "Render/OUniformBuffer.h"
#include "Render/OVertexArrayObject.h"

void ORenderEngine::Clear(const OVector4& color)
{
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ORenderEngine::SetFaceCulling(const OCullingType& type)
{
	auto cullType = GL_BACK;

	if (type == OCullingType::FrontFace) cullType = GL_FRONT;
	else if (type == OCullingType::BackFace) cullType = GL_BACK;
	else if (type == OCullingType::Both) cullType = GL_FRONT_AND_BACK;

	glEnable(GL_CULL_FACE);
	glCullFace(cullType);
	glEnable(GL_DEPTH_TEST);
}

void ORenderEngine::SetWindingOrder(const OWindingOrder& order)
{
	auto orderType = GL_CW;

	if (order == ClockWise) orderType = GL_CW;
	else if (order == CounterClockWise) orderType = GL_CCW;

	glFrontFace(orderType);
}

void ORenderEngine::SetViewPort(const ORect& rect)
{
	glViewport(rect.left, rect.top, rect.width, rect.height);
}

void ORenderEngine::SetVertexArrayObject(const OVertexArrayObjectPtr& vao)
{
	glBindVertexArray(vao->GetId());
}

void ORenderEngine::SetShaderProgram(const OShaderProgramPtr& program)
{
	glUseProgram(program->GetId());
}

void ORenderEngine::SetUniformBuffer(const OUniformBufferPtr& buffer, unsigned int slot)
{
	glBindBufferBase(GL_UNIFORM_BUFFER,slot, buffer->GetId());
}

void ORenderEngine::DrawTriangles(const OTriangleType& type, unsigned int vertexCount,unsigned int offset)
{
	auto glTriType = GL_TRIANGLES;

	switch (type)
	{
		case OTriangleType::List:{ glTriType = GL_TRIANGLES; break; }
		case OTriangleType::Strip: { glTriType = GL_TRIANGLE_STRIP; break; }
	}

	glDrawArrays(glTriType, offset, vertexCount);
}

void ORenderEngine::DrawIndexedTriangles(const OTriangleType& type, unsigned int indicesCount)
{
	auto glTriType = GL_TRIANGLES;

	switch (type)
	{
	case List: { glTriType = GL_TRIANGLES; break; }
	case Strip: { glTriType = GL_TRIANGLE_STRIP; break; }
	}

	glDrawElements(glTriType, indicesCount, GL_UNSIGNED_INT, nullptr);
}

OVertexArrayObjectPtr ORenderEngine::CreateVertexArrayObject(const OVertexBufferDesc& desc)
{
	return std::make_shared<OVertexArrayObject>(desc);
}

OVertexArrayObjectPtr ORenderEngine::CreateVertexArrayObject(const OVertexBufferDesc& vertexBufferDesc, const OIndexBufferDesc& indexBufferDesc)
{
	return std::make_shared<OVertexArrayObject>(vertexBufferDesc, indexBufferDesc);
}

OShaderProgramPtr ORenderEngine::CreateShaderProgram(const OShaderProgramDesc& desc)
{
	return std::make_shared<OShaderProgram>(desc);
}

OUniformBufferPtr ORenderEngine::CreateUniformBuffer(const OUniformBufferDesc& desc)
{
	return std::make_shared<OUniformBuffer>(desc);
}

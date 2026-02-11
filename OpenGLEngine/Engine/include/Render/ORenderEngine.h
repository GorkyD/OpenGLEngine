#pragma once
#include "Extension/OExtension.h"
#include "Math/ORect.h"
#include "Math/OVector4.h"

class ORenderEngine
{
	public:
		ORenderEngine();
		~ORenderEngine();

		OVertexArrayObjectPtr CreateVertexArrayObject(const OVertexBufferDesc& desc);
		OVertexArrayObjectPtr CreateVertexArrayObject(const OVertexBufferDesc& vertexBufferDesc, const OIndexBufferDesc& indexBufferDesc);
		OShaderProgramPtr CreateShaderProgram(const OShaderProgramDesc& desc);
		OUniformBufferPtr CreateUniformBuffer(const OUniformBufferDesc& desc);


		void Clear(const OVector4& color);
		void SetFaceCulling(const OCullingType& type);
		void SetWindingOrder(const OWindingOrder& order);
		void SetViewPort(const ORect& rect);
		void SetVertexArrayObject(const OVertexArrayObjectPtr& vao);
		void SetShaderProgram(const OShaderProgramPtr& program);
		void SetUniformBuffer(const OUniformBufferPtr& buffer, unsigned int slot);
		void DrawTriangles(const OTriangleType& type, unsigned int vertexCount,unsigned int offset);
		void DrawIndexedTriangles(const OTriangleType& type, unsigned int indicesCount);
};


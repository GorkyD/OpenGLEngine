#pragma once
#include "Extension/Extension.h"
#include "Math/Rect.h"
#include "Math/Vector4.h"

class RenderEngine
{
	public:
		RenderEngine();
		~RenderEngine();

		VertexArrayObjectPtr CreateVertexArrayObject(const VertexBufferDesc& desc);
		VertexArrayObjectPtr CreateVertexArrayObject(const VertexBufferDesc& vertexBufferDesc, const IndexBufferDesc& indexBufferDesc);
		ShaderProgramPtr CreateShaderProgram(const ShaderProgramDesc& desc);
		UniformBufferPtr CreateUniformBuffer(const UniformBufferDesc& desc);


		void Clear(const Vector4& color);
		void SetFaceCulling(const CullingType& type);
		void SetWindingOrder(const WindingOrder& order);
		void SetViewPort(const Rect& rect);
		void SetVertexArrayObject(const VertexArrayObjectPtr& vao);
		void SetShaderProgram(const ShaderProgramPtr& program);
		void SetUniformBuffer(const UniformBufferPtr& buffer, unsigned int slot);
		void DrawTriangles(const TriangleType& type, unsigned int vertexCount,unsigned int offset);
		void DrawIndexedTriangles(const TriangleType& type, unsigned int indicesCount);
};

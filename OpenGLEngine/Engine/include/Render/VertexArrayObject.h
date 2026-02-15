#pragma once
#include "Extension/Extension.h"

class VertexArrayObject
{
	public:
		VertexArrayObject(const VertexBufferDesc& vertexBufferDesc);
		VertexArrayObject(const VertexBufferDesc& vertexBufferDesc, const IndexBufferDesc& indexBufferDesc);
		~VertexArrayObject();

		unsigned int GetId() const;

		unsigned int GetVertexBufferSize() const;
		unsigned int GetVertexSize() const;

	private:
		VertexBufferDesc vertexBufferDescData;

		unsigned int elementBufferId = 0;
		unsigned int vertexBufferId = 0;
		unsigned int vertexArrayObjectId = 0;
};

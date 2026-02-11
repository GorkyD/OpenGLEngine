#pragma once
#include "Extension/OExtension.h"

class OVertexArrayObject
{
	public:
		OVertexArrayObject(const OVertexBufferDesc& vertexBufferDesc);
		OVertexArrayObject(const OVertexBufferDesc& vertexBufferDesc, const OIndexBufferDesc& indexBufferDesc);
		~OVertexArrayObject();

		unsigned int GetId() const;

		unsigned int GetVertexBufferSize() const;
		unsigned int GetVertexSize() const;

	private:
		OVertexBufferDesc vertexBufferDescData;

		unsigned int elementBufferId = 0;
		unsigned int vertexBufferId = 0;
		unsigned int vertexArrayObjectId = 0;
};


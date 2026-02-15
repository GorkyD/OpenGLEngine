#include "Render/VertexArrayObject.h"
#include <glad/glad.h>

VertexArrayObject::VertexArrayObject(const VertexBufferDesc& vertexBufferDesc)
{
	if(!vertexBufferDesc.listSize) OGL_ERROR("VertexArrayObject | vertexSize is NULL")
	if (!vertexBufferDesc.vertexSize) OGL_ERROR("VertexArrayObject | vertexSize is NULL")
	if (!vertexBufferDesc.verticesList) OGL_ERROR("VertexArrayObject | verticesList is NULL")

	glGenVertexArrays(1, &vertexArrayObjectId);
	glBindVertexArray(vertexArrayObjectId);

	glGenBuffers(1,&vertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferDesc.vertexSize * vertexBufferDesc.listSize, vertexBufferDesc.verticesList, GL_STATIC_DRAW);


	for (int i = 0; i < vertexBufferDesc.attributesListSize; i++) 
	{
		glVertexAttribPointer(
			i, 
			vertexBufferDesc.attributesList[i].numElements,
			GL_FLOAT, 
			GL_FALSE,
			vertexBufferDesc.vertexSize, 
			(void*)((i == 0) ? 0 : vertexBufferDesc.attributesList[i - 1].numElements * sizeof(float)));

		glEnableVertexAttribArray(i);
	}

	glBindVertexArray(0);

	vertexBufferDescData = vertexBufferDesc;
}

VertexArrayObject::VertexArrayObject(const VertexBufferDesc& vertexBufferDesc, const IndexBufferDesc& indexBufferDesc) : VertexArrayObject(vertexBufferDesc)
{
	if (!indexBufferDesc.listSize) OGL_ERROR("VertexArrayObject | listSize is NULL")
	if (!indexBufferDesc.indicesList) OGL_ERROR("VertexArrayObject | indicesList is NULL")

	glBindVertexArray(vertexArrayObjectId);

	glGenBuffers(1, &elementBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferDesc.listSize, indexBufferDesc.indicesList, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

VertexArrayObject::~VertexArrayObject()
{
	glDeleteBuffers(1, &elementBufferId);
	glDeleteBuffers(1, &vertexBufferId);
	glDeleteVertexArrays(1, &vertexArrayObjectId);
}

unsigned int VertexArrayObject::GetId() const
{
	return vertexArrayObjectId;
}

unsigned int VertexArrayObject::GetVertexBufferSize() const
{
	return vertexBufferDescData.listSize;
}

unsigned int VertexArrayObject::GetVertexSize() const
{
	return vertexBufferDescData.vertexSize;
}

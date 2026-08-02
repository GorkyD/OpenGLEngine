#include "Render/VertexArrayObject.h"
#include <glad/glad.h>

VertexArrayObject::VertexArrayObject(const VertexBufferDesc& vertexBufferDesc)
{
    if (!vertexBufferDesc.listSize)
        OGL_ERROR("VertexArrayObject | vertexSize is NULL")
    if (!vertexBufferDesc.vertexSize)
        OGL_ERROR("VertexArrayObject | vertexSize is NULL")
    if (!vertexBufferDesc.verticesList)
        OGL_ERROR("VertexArrayObject | verticesList is NULL")

    glGenVertexArrays(1, &vertexArrayObjectId);
    glBindVertexArray(vertexArrayObjectId);

    glGenBuffers(1, &vertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferDesc.vertexSize * vertexBufferDesc.listSize, vertexBufferDesc.verticesList, GL_STATIC_DRAW);

    unsigned int offset = 0;
    for (int i = 0; i < vertexBufferDesc.attributesListSize; i++)
    {
        glVertexAttribPointer(i, vertexBufferDesc.attributesList[i].numElements, GL_FLOAT, GL_FALSE, vertexBufferDesc.vertexSize, reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
        glEnableVertexAttribArray(i);
        offset += vertexBufferDesc.attributesList[i].numElements * sizeof(float);
    }

    glBindVertexArray(0);

    vertexBufferDescData = vertexBufferDesc;
}

VertexArrayObject::VertexArrayObject(const VertexBufferDesc& vertexBufferDesc, const IndexBufferDesc& indexBufferDesc) : VertexArrayObject(vertexBufferDesc)
{
    if (!indexBufferDesc.listSize)
        OGL_ERROR("VertexArrayObject | listSize is NULL")
    if (!indexBufferDesc.indicesList)
        OGL_ERROR("VertexArrayObject | indicesList is NULL")

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
    if (instanceBufferId)
        glDeleteBuffers(1, &instanceBufferId);
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

void VertexArrayObject::UploadInstanceData(const InstanceData* data, unsigned int count)
{
    if (count == 0)
        return;

    if (!instanceBufferId)
        glGenBuffers(1, &instanceBufferId);

    glBindVertexArray(vertexArrayObjectId);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBufferId);

    if (count > instanceBufferCapacity)
    {
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(InstanceData), data, GL_DYNAMIC_DRAW);
        instanceBufferCapacity = count;

        const unsigned int stride = sizeof(InstanceData);
        for (int i = 0; i < 4; i++)
        {
            const int location = 3 + i;
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(i * 16)));
            glVertexAttribDivisor(location, 1);
        }

        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(64)));
        glVertexAttribDivisor(7, 1);

        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(80)));
        glVertexAttribDivisor(8, 1);

        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(static_cast<uintptr_t>(96)));
        glVertexAttribDivisor(9, 1);
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(InstanceData), data);
    }

    glBindVertexArray(0);
}

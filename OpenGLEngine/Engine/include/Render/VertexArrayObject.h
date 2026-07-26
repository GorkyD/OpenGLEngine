#pragma once

#include "Extension/Extension.h"
#include "Render/InstanceData.h"

class VertexArrayObject
{
public:
    VertexArrayObject(const VertexBufferDesc& vertexBufferDesc);
    VertexArrayObject(const VertexBufferDesc& vertexBufferDesc, const IndexBufferDesc& indexBufferDesc);
    ~VertexArrayObject();

    unsigned int GetId() const;
    unsigned int GetVertexBufferSize() const;
    unsigned int GetVertexSize() const;

    void UploadInstanceData(const InstanceData* data, unsigned int count);

private:
    VertexBufferDesc vertexBufferDescData;

    unsigned int elementBufferId = 0;
    unsigned int vertexBufferId = 0;
    unsigned int vertexArrayObjectId = 0;

    unsigned int instanceBufferId = 0;
    unsigned int instanceBufferCapacity = 0;
};

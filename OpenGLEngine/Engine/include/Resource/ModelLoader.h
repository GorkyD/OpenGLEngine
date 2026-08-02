#pragma once

#include <string>
#include <vector>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

struct MeshVertex
{
    Vector3 position;
    Vector2 uv;
    Vector3 normal;
};

struct MeshData
{
    std::string name;
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;
    int materialIndex = -1;
};

struct MaterialData
{
    std::string diffuseTexturePath;
    Vector4 diffuseColor = {1, 1, 1, 1};

    std::vector<unsigned char> embeddedTexture;
    int embeddedWidth = 0;
    int embeddedHeight = 0;
};

struct ModelData
{
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
};

class ModelLoader
{
public:
    static ModelData Load(const std::string& filePath);
};

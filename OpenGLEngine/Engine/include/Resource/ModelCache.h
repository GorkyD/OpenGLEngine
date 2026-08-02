#pragma once

#include <string>
#include <vector>
#include "Extension/Extension.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class RenderEngine;

struct CachedMesh
{
    VertexArrayObjectPtr vao;
    unsigned int indexCount = 0;
    int materialIndex = -1;
    std::string name;

    Vector3 boundsMin = {0, 0, 0};
    Vector3 boundsMax = {0, 0, 0};
    bool hasBounds = false;
};

struct CachedModel
{
    std::vector<CachedMesh> meshes;
    std::vector<TexturePtr> textures;
    std::vector<Vector4> diffuseColors;
};

class ModelCache
{
public:
    static const CachedModel& Get(RenderEngine* renderEngine, const std::string& path);
    static void Clear();
};

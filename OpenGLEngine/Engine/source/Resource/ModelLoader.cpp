#include "Resource/ModelLoader.h"
#include "Extension/Extension.h"
#include "Math/Matrix4.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static Matrix4 ConvertMatrix(const aiMatrix4x4& m)
{
    Matrix4 result;
    result.matrix[0][0] = m.a1;
    result.matrix[0][1] = m.b1;
    result.matrix[0][2] = m.c1;
    result.matrix[0][3] = m.d1;
    result.matrix[1][0] = m.a2;
    result.matrix[1][1] = m.b2;
    result.matrix[1][2] = m.c2;
    result.matrix[1][3] = m.d2;
    result.matrix[2][0] = m.a3;
    result.matrix[2][1] = m.b3;
    result.matrix[2][2] = m.c3;
    result.matrix[2][3] = m.d3;
    result.matrix[3][0] = m.a4;
    result.matrix[3][1] = m.b4;
    result.matrix[3][2] = m.c4;
    result.matrix[3][3] = m.d4;
    return result;
}

static Vector3 TransformPoint(const Vector3& v, const Matrix4& m)
{
    return {v.x * m.matrix[0][0] + v.y * m.matrix[1][0] + v.z * m.matrix[2][0] + m.matrix[3][0],
            v.x * m.matrix[0][1] + v.y * m.matrix[1][1] + v.z * m.matrix[2][1] + m.matrix[3][1],
            v.x * m.matrix[0][2] + v.y * m.matrix[1][2] + v.z * m.matrix[2][2] + m.matrix[3][2]};
}

static Vector3 TransformDirection(const Vector3& v, const Matrix4& m)
{
    return {v.x * m.matrix[0][0] + v.y * m.matrix[1][0] + v.z * m.matrix[2][0], v.x * m.matrix[0][1] + v.y * m.matrix[1][1] + v.z * m.matrix[2][1],
            v.x * m.matrix[0][2] + v.y * m.matrix[1][2] + v.z * m.matrix[2][2]};
}

static MeshData ProcessMesh(aiMesh* mesh, const Matrix4& nodeTransform)
{
    MeshData data;
    data.name = mesh->mName.C_Str();
    data.materialIndex = mesh->mMaterialIndex;

    data.vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        MeshVertex v;
        v.position = TransformPoint({mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z}, nodeTransform);

        if (mesh->mTextureCoords[0])
            v.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        else
            v.uv = {0, 0};

        if (mesh->mNormals)
            v.normal = Vector3::Normalize(TransformDirection({mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z}, nodeTransform));
        else
            v.normal = {0, 1, 0};

        data.vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            data.indices.push_back(face.mIndices[j]);
    }

    return data;
}

static void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshData>& meshes, const Matrix4& parentTransform)
{
    const Matrix4 nodeTransform = ConvertMatrix(node->mTransformation) * parentTransform;

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
        meshes.push_back(ProcessMesh(scene->mMeshes[node->mMeshes[i]], nodeTransform));

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessNode(node->mChildren[i], scene, meshes, nodeTransform);
}

ModelData ModelLoader::Load(const std::string& filePath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        OGL_ERROR("ModelLoader | " << importer.GetErrorString())
    }

    ModelData model;

    std::string directory = filePath.substr(0, filePath.find_last_of("/\\") + 1);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* mat = scene->mMaterials[i];
        MaterialData matData;

        aiColor4D color;
        if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
            matData.diffuseColor = {color.r, color.g, color.b, color.a};

        if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString texPath;
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

            const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.C_Str());
            if (embedded)
            {
                if (embedded->mHeight == 0)
                {
                    const auto* bytes = reinterpret_cast<const unsigned char*>(embedded->pcData);
                    matData.embeddedTexture.assign(bytes, bytes + embedded->mWidth);
                }
                else
                {
                    matData.embeddedWidth = static_cast<int>(embedded->mWidth);
                    matData.embeddedHeight = static_cast<int>(embedded->mHeight);
                    matData.embeddedTexture.reserve(static_cast<size_t>(embedded->mWidth) * embedded->mHeight * 4);

                    const unsigned int texelCount = embedded->mWidth * embedded->mHeight;
                    for (unsigned int t = 0; t < texelCount; t++)
                    {
                        matData.embeddedTexture.push_back(embedded->pcData[t].r);
                        matData.embeddedTexture.push_back(embedded->pcData[t].g);
                        matData.embeddedTexture.push_back(embedded->pcData[t].b);
                        matData.embeddedTexture.push_back(embedded->pcData[t].a);
                    }
                }
            }
            else
            {
                matData.diffuseTexturePath = directory + texPath.C_Str();
            }
        }

        model.materials.push_back(matData);
    }

    ProcessNode(scene->mRootNode, scene, model.meshes, Matrix4());

    OGL_INFO("ModelLoader | Loaded: " << filePath << model.meshes.size() << " meshes, " << model.materials.size() << " materials)")

    return model;
}

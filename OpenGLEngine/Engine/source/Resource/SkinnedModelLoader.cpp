#include "Resource/SkinnedModelLoader.h"
#include "Extension/Extension.h"
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

static void BuildSkeletonNodes(aiNode* node, int parentIndex, Skeleton& skeleton)
{
    SkeletonNode skelNode;
    skelNode.name = node->mName.C_Str();
    skelNode.localBindTransform = ConvertMatrix(node->mTransformation);
    skelNode.parentIndex = parentIndex;

    aiVector3D bindScale, bindPosition;
    aiQuaternion bindRotation;
    node->mTransformation.Decompose(bindScale, bindRotation, bindPosition);
    skelNode.bindPosition = Vector3(bindPosition.x, bindPosition.y, bindPosition.z);
    skelNode.bindRotation = Quaternion(bindRotation.x, bindRotation.y, bindRotation.z, bindRotation.w);
    skelNode.bindScale = Vector3(bindScale.x, bindScale.y, bindScale.z);

    const int nodeIndex = static_cast<int>(skeleton.nodes.size());
    skeleton.nodes.push_back(skelNode);
    skeleton.nodeNameToIndex[skelNode.name] = nodeIndex;

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        BuildSkeletonNodes(node->mChildren[i], nodeIndex, skeleton);
}

static SkinnedMeshData ProcessSkinnedMesh(aiMesh* mesh, Skeleton& skeleton)
{
    SkinnedMeshData data;
    data.materialIndex = mesh->mMaterialIndex;

    data.vertices.resize(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        SkinnedVertex& v = data.vertices[i];
        v.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        if (mesh->mTextureCoords[0])
            v.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
        else
            v.uv = {0, 0};

        if (mesh->mNormals)
            v.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        else
            v.normal = {0, 1, 0};
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            data.indices.push_back(face.mIndices[j]);
    }

    for (unsigned int b = 0; b < mesh->mNumBones; b++)
    {
        aiBone* bone = mesh->mBones[b];
        const std::string boneName = bone->mName.C_Str();

        int boneIndex;
        const auto it = skeleton.boneNameToIndex.find(boneName);
        if (it != skeleton.boneNameToIndex.end())
        {
            boneIndex = it->second;
        }
        else
        {
            BoneInfo info;
            info.name = boneName;
            info.offsetMatrix = ConvertMatrix(bone->mOffsetMatrix);
            const auto nodeIt = skeleton.nodeNameToIndex.find(boneName);
            info.nodeIndex = nodeIt != skeleton.nodeNameToIndex.end() ? nodeIt->second : -1;

            boneIndex = static_cast<int>(skeleton.bones.size());
            skeleton.bones.push_back(info);
            skeleton.boneNameToIndex[boneName] = boneIndex;
        }

        for (unsigned int w = 0; w < bone->mNumWeights; w++)
        {
            const unsigned int vertexId = bone->mWeights[w].mVertexId;
            const float weight = bone->mWeights[w].mWeight;
            if (weight <= 0.0f || vertexId >= data.vertices.size())
                continue;

            SkinnedVertex& v = data.vertices[vertexId];
            for (int slot = 0; slot < 4; slot++)
            {
                if (v.boneWeights[slot] == 0.0f)
                {
                    v.boneIds[slot] = static_cast<float>(boneIndex);
                    v.boneWeights[slot] = weight;
                    break;
                }
            }
        }
    }

    for (auto& v : data.vertices)
    {
        const float sum = v.boneWeights[0] + v.boneWeights[1] + v.boneWeights[2] + v.boneWeights[3];
        if (sum > 0.0001f)
        {
            const float inv = 1.0f / sum;
            for (int k = 0; k < 4; k++)
                v.boneWeights[k] *= inv;
        }
    }

    return data;
}

static void ProcessMeshNode(aiNode* node, const aiScene* scene, Skeleton& skeleton, std::vector<SkinnedMeshData>& meshes)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
        meshes.push_back(ProcessSkinnedMesh(scene->mMeshes[node->mMeshes[i]], skeleton));

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessMeshNode(node->mChildren[i], scene, skeleton, meshes);
}

SkinnedModelData SkinnedModelLoader::Load(const std::string& filePath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights | aiProcess_FlipWindingOrder);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        OGL_ERROR("SkinnedModelLoader | " << importer.GetErrorString())
    }

    SkinnedModelData model;

    BuildSkeletonNodes(scene->mRootNode, -1, model.skeleton);

    Matrix4 mirrorZ;
    mirrorZ.matrix[2][2] = -1.0f;
    model.skeleton.globalInverseTransform = ConvertMatrix(scene->mRootNode->mTransformation).Inverse() * mirrorZ;

    const std::string directory = filePath.substr(0, filePath.find_last_of("/\\") + 1);

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
            matData.diffuseTexturePath = directory + texPath.C_Str();
        }

        model.materials.push_back(matData);
    }

    ProcessMeshNode(scene->mRootNode, scene, model.skeleton, model.meshes);

    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        aiAnimation* anim = scene->mAnimations[i];

        AnimationClip clip;
        clip.name = anim->mName.C_Str();
        clip.durationTicks = static_cast<float>(anim->mDuration);
        clip.ticksPerSecond = anim->mTicksPerSecond != 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;

        for (unsigned int c = 0; c < anim->mNumChannels; c++)
        {
            aiNodeAnim* channel = anim->mChannels[c];

            AnimationChannel ac;
            ac.nodeName = channel->mNodeName.C_Str();

            for (unsigned int k = 0; k < channel->mNumPositionKeys; k++)
            {
                const auto& key = channel->mPositionKeys[k];
                ac.positionKeys.push_back({static_cast<float>(key.mTime), Vector3(key.mValue.x, key.mValue.y, key.mValue.z)});
            }

            for (unsigned int k = 0; k < channel->mNumRotationKeys; k++)
            {
                const auto& key = channel->mRotationKeys[k];
                ac.rotationKeys.push_back({static_cast<float>(key.mTime), Quaternion(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w)});
            }

            for (unsigned int k = 0; k < channel->mNumScalingKeys; k++)
            {
                const auto& key = channel->mScalingKeys[k];
                ac.scaleKeys.push_back({static_cast<float>(key.mTime), Vector3(key.mValue.x, key.mValue.y, key.mValue.z)});
            }

            clip.channels.push_back(ac);
        }

        model.animations.push_back(clip);
    }

    OGL_INFO("SkinnedModelLoader | Loaded: " << filePath << " (" << model.meshes.size() << " meshes, " << model.skeleton.bones.size() << " bones, " << model.animations.size()
                                              << " animations)")

    return model;
}

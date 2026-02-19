#include "Resource/ModelLoader.h"
#include "Extension/Extension.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

static MeshData ProcessMesh(aiMesh* mesh)
{
	MeshData data;
	data.materialIndex = mesh->mMaterialIndex;

	data.vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		MeshVertex v;
		v.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

		if (mesh->mTextureCoords[0])
			v.uv = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
		else
			v.uv = {0, 0};

		if (mesh->mNormals)
			v.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
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

static void ProcessNode(aiNode* node, const aiScene* scene, std::vector<MeshData>& meshes)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
		meshes.push_back(ProcessMesh(scene->mMeshes[node->mMeshes[i]]));

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		ProcessNode(node->mChildren[i], scene, meshes);
}

ModelData ModelLoader::Load(const std::string& filePath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace);

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
	{
		OGL_ERROR("ModelLoader | " , importer.GetErrorString());
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
			matData.diffuseTexturePath = directory + texPath.C_Str();
		}

		model.materials.push_back(matData);
	}

	ProcessNode(scene->mRootNode, scene, model.meshes);

	OGL_INFO("ModelLoader | Loaded: " , filePath
		, " (" , model.meshes.size() , " meshes, "
		, model.materials.size() , " materials)");

	return model;
}

#include "model_resource_file.h"

#include "resource_loader.h"
#include "image_resource_file.h"
#include "renderer/mesh.h"
#include "renderer/vertex_buffer.h"
#include "renderer/index_buffer.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "meshoptimizer.h"

ModelResourceFile::ModelResourceFile(const FilePath& path)
    : ResourceFile(Type::Model, path)
{
	reimport();
}

void ModelResourceFile::reimport()
{
	ResourceFile::reimport();

	Assimp::Importer modelLoader;
	const aiScene* scene = modelLoader.ReadFile(
	    getPath().generic_string(),
	    aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SplitLargeMeshes | aiProcess_GenBoundingBoxes | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace | aiProcess_RemoveComponent | aiProcess_PreTransformVertices);

	if (!scene)
	{
		ERR("Model could not be loaded: " + getPath().generic_string());
		ERR("Assimp: " + modelLoader.GetErrorString());
		return;
	}

	m_Meshes.clear();
	for (int i = 0; i < scene->mNumMeshes; i++)
	{
		const aiMesh* mesh = scene->mMeshes[i];

		Vector<VertexData> vertices;
		vertices.reserve(mesh->mNumVertices);

		VertexData vertex;
		ZeroMemory(&vertex, sizeof(VertexData));
		for (unsigned int v = 0; v < mesh->mNumVertices; v++)
		{
			vertex.position.x = mesh->mVertices[v].x;
			vertex.position.y = mesh->mVertices[v].y;
			vertex.position.z = mesh->mVertices[v].z;

			if (mesh->mNormals)
			{
				vertex.normal.x = mesh->mNormals[v].x;
				vertex.normal.y = mesh->mNormals[v].y;
				vertex.normal.z = mesh->mNormals[v].z;
			}

			if (mesh->mTextureCoords)
			{
				if (mesh->mTextureCoords[0])
				{
					// Assuming the model has texture coordinates and taking only the first texture coordinate in case of multiple texture coordinates
					vertex.textureCoord.x = mesh->mTextureCoords[0][v].x;
					vertex.textureCoord.y = mesh->mTextureCoords[0][v].y;
				}
			}

			if (mesh->mTangents)
			{
				vertex.tangent.x = mesh->mTangents[v].x;
				vertex.tangent.y = mesh->mTangents[v].y;
				vertex.tangent.z = mesh->mTangents[v].z;
			}

			vertices.push_back(vertex);
		}

		Vector<unsigned int> indices;
		indices.reserve(mesh->mNumFaces);

		aiFace* face = nullptr;
		for (unsigned int f = 0; f < mesh->mNumFaces; f++)
		{
			face = &mesh->mFaces[f];
			//Model already triangulated by aiProcess_Triangulate so no need to check
			indices.push_back(face->mIndices[0]);
			indices.push_back(face->mIndices[1]);
			indices.push_back(face->mIndices[2]);
		}

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

		Vector<Vector<unsigned int>> lods;
		float lodLevels[MAX_LOD_COUNT - 1] = { 0.8f, 0.50f, 0.3f, 0.10f };

		for (int i = 0; i < MAX_LOD_COUNT - 1; i++)
		{
			float threshold = lodLevels[i];
			size_t targetIndexCount = indices.size() * threshold;

			Vector<unsigned int> lod(indices.size());
			float lodError = 0.0f;
			size_t finalLODIndexCount = meshopt_simplifySloppy(
			    &lod[0],
			    indices.data(),
			    indices.size(),
			    &vertices[0].position.x,
			    vertices.size(),
			    sizeof(VertexData),
			    targetIndexCount);
			lod.resize(finalLODIndexCount);

			lods.push_back(lod);
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		aiColor3D color(1.0f, 1.0f, 1.0f);
		float alpha = 1.0f;
		if (AI_SUCCESS != material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
		{
			WARN("Material does not have color: " + String(material->GetName().C_Str()));
		}
		if (AI_SUCCESS != material->Get(AI_MATKEY_OPACITY, alpha))
		{
			WARN("Material does not have alpha: " + String(material->GetName().C_Str()));
		}

		Ref<BasicMaterialResourceFile> extractedMaterial;

		String materialPath(material->GetName().C_Str());
		if (materialPath == "DefaultMaterial" || materialPath == "None" || materialPath.empty())
		{
			materialPath = "rootex/assets/materials/default.basic.rmat";
		}
		else
		{
			String dir = "game/assets/materials/" + getPath().stem().generic_string();
			if (!OS::IsExists(dir))
			{
				OS::CreateDirectoryName(dir);
			}

			materialPath = dir + "/" + String(material->GetName().C_Str()) + ".basic.rmat";
		}

		if (OS::IsExists(materialPath))
		{
			extractedMaterial = ResourceLoader::CreateBasicMaterialResourceFile(materialPath);
		}
		else
		{
			extractedMaterial = ResourceLoader::CreateNewBasicMaterialResourceFile(materialPath);

			extractedMaterial->setColor({ color.r, color.g, color.b, alpha });

			for (int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++)
			{
				aiString diffuseStr;
				material->GetTexture(aiTextureType_DIFFUSE, i, &diffuseStr);

				bool isEmbedded = *diffuseStr.C_Str() == '*';
				if (!isEmbedded)
				{
					// Texture is given as a path
					String texturePath = diffuseStr.C_Str();
					Ref<ImageResourceFile> image = ResourceLoader::CreateImageResourceFile(getPath().parent_path().generic_string() + "/" + texturePath);

					if (image)
					{
						extractedMaterial->setDiffuse(image);
					}
					else
					{
						WARN("Could not set material diffuse image: " + texturePath);
					}
				}
				else
				{
					WARN("Embedded diffuse texture found in material: " + materialPath + ". Embedded textures are unsupported.");
				}
			}

			for (int i = 0; i < material->GetTextureCount(aiTextureType_NORMALS); i++)
			{
				aiString normalStr;
				material->GetTexture(aiTextureType_NORMALS, i, &normalStr);
				bool isEmbedded = *normalStr.C_Str() == '*';
				if (isEmbedded)
				{
					String texturePath = normalStr.C_Str();
					Ref<ImageResourceFile> image = ResourceLoader::CreateImageResourceFile(getPath().parent_path().generic_string() + "/" + texturePath);

					if (image)
					{
						extractedMaterial->setNormal(image);
					}
					else
					{
						WARN("Could not set material normal map texture: " + texturePath);
					}
				}
				else
				{
					WARN("Embedded normal texture found in material: " + materialPath + ". Embedded textures are unsupported.");
				}
			}

			for (int i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); i++)
			{
				aiString specularStr;
				material->GetTexture(aiTextureType_SPECULAR, i, &specularStr);
				bool isEmbedded = *specularStr.C_Str() == '*';
				if (isEmbedded)
				{
					String texturePath = specularStr.C_Str();
					Ref<ImageResourceFile> image = ResourceLoader::CreateImageResourceFile(getPath().parent_path().generic_string() + "/" + texturePath);

					if (image)
					{
						extractedMaterial->setSpecular(image);
					}
					else
					{
						WARN("Could not set material specular map texture: " + texturePath);
					}
				}
				else
				{
					WARN("Embedded specular texture found in material: " + materialPath + ". Embedded textures are unsupported.");
				}
			}

			for (int i = 0; i < material->GetTextureCount(aiTextureType_LIGHTMAP); i++)
			{
				aiString lightmapStr;
				material->GetTexture(aiTextureType_LIGHTMAP, i, &lightmapStr);
				bool isEmbedded = *lightmapStr.C_Str() == '*';
				if (isEmbedded)
				{
					String texturePath = lightmapStr.C_Str();
					Ref<ImageResourceFile> image = ResourceLoader::CreateImageResourceFile(getPath().parent_path().generic_string() + "/" + texturePath);

					if (image)
					{
						extractedMaterial->setLightmap(image);
					}
					else
					{
						WARN("Could not set material lightmap texture: " + texturePath);
					}
				}
				else
				{
					WARN("Embedded lightmaptexture found in material: " + materialPath + ". Embedded textures are unsupported.");
				}
			}
		}

		Mesh extractedMesh;
		extractedMesh.m_VertexBuffer.reset(new VertexBuffer((const char*)vertices.data(), vertices.size(), sizeof(VertexData), D3D11_USAGE_IMMUTABLE, 0));
		extractedMesh.addLOD(std::make_shared<IndexBuffer>(indices), 1.0f);
		for (int i = 0; i < MAX_LOD_COUNT - 1; i++)
		{
			if (!lods[i].empty())
			{
				extractedMesh.addLOD(std::make_shared<IndexBuffer>(lods[i]), lodLevels[i]);
			}
		}
		Vector3 max = { mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z };
		Vector3 min = { mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z };
		Vector3 center = (max + min) / 2.0f;
		extractedMesh.m_BoundingBox.Center = center;
		extractedMesh.m_BoundingBox.Extents = (max - min) / 2.0f;
		extractedMesh.m_BoundingBox.Extents.x = abs(extractedMesh.m_BoundingBox.Extents.x);
		extractedMesh.m_BoundingBox.Extents.y = abs(extractedMesh.m_BoundingBox.Extents.y);
		extractedMesh.m_BoundingBox.Extents.z = abs(extractedMesh.m_BoundingBox.Extents.z);

		bool found = false;
		for (auto& materialModels : getMeshes())
		{
			if (materialModels.first == extractedMaterial)
			{
				found = true;
				materialModels.second.push_back(extractedMesh);
				break;
			}
		}

		if (!found && extractedMaterial)
		{
			getMeshes().push_back(Pair<Ref<BasicMaterialResourceFile>, Vector<Mesh>>(extractedMaterial, { extractedMesh }));
		}
	}
}

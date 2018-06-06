#define TINYOBJLOADER_IMPLEMENTATION
#include "model.h"

model::model()
{
}


model::~model()
{
}

void model::loadModel(std::string modelPath)
{/*
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str()))	throw std::runtime_error(err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices;

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
	*/
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::exception("failed to load model");

	aiMesh* mesh = scene->mMeshes[0];

	// load all materials . materials are saved in scene. Maaterial shold be in model folder
	/*for (int i = 0; i < scene->mNumMaterials; i++)
	{
		aiString str;
		scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &str);
		if (str.data[0] != '\0')
		{
			tex = loadTexture(path.substr(0, path.find_last_of('\\')) + "\\" + str.data);
		}
	}
	// if no textures are in model use random color
	if (tex.name == "")*/
	{
		std::random_device randGen;
		float n1, n2, n3;
		n1 = glm::abs(randGen()) / (float)randGen.max();
		n2 = glm::abs(randGen()) / (float)randGen.max();
		n3 = glm::abs(randGen()) / (float)randGen.max();

		mColor = glm::vec3(n1, n2, n3);
		printf("\n %f,%f,%f", n1, n2, n3);
	}

	const uint8_t verticesPerFace = 3;
	mIndices.resize(mesh->mNumFaces * verticesPerFace);
	for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (uint32_t j = 0; j < verticesPerFace; ++j)
		{
			mIndices[(i*verticesPerFace) + j] = face.mIndices[j];
		}
	}

	mVertices.resize(mesh->mNumVertices);
	for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		mVertices[i].pos.x = mesh->mVertices[i].x;
		mVertices[i].pos.y = mesh->mVertices[i].y;
		mVertices[i].pos.z = mesh->mVertices[i].z;

		mVertices[i].texCoord.x = mesh->mTextureCoords[0][i].x;
		mVertices[i].texCoord.y = mesh->mTextureCoords[0][i].y;

		mVertices[i].color = mColor;
	}
}

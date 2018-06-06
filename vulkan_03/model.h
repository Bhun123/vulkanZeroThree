#pragma once

#include <unordered_map>
#include <string>
#include <random>

#include "rendererStructs.h"

#include <tiny_obj_loader.h>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

class model
{
public:
	model();
	~model();

	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices; // dont forget t o change command buffer rendering to 32 bit
	// just use obj loader - simple first for vulkan learning
	void loadModel(std::string modelPath);

	glm::vec3 mColor = { 0.0f, 0.0f, 0.0f };
};


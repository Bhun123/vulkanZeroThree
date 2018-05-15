#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription();
	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct bufferAllocation
{
	VkBuffer buffer;
	VmaAllocation allocation;
};

struct imageAllocation
{
	VkImage image;
	VmaAllocation allocation;
};
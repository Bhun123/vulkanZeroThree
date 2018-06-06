#define GLM_ENABLE_EXPERIMENTAL
#include "rendererStructs.h"

vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
	return vk::VertexInputBindingDescription()
		.setBinding(0)
		.setStride(sizeof(Vertex))
		.setInputRate(vk::VertexInputRate::eVertex);
}

std::array<vk::VertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions()
{
	std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};
	attributeDescriptions[0].setBinding(0);
	attributeDescriptions[0].setLocation(0);
	attributeDescriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
	attributeDescriptions[0].setOffset(offsetof(Vertex, pos));

	attributeDescriptions[1].setBinding(0);
	attributeDescriptions[1].setLocation(1);
	attributeDescriptions[1].setFormat(vk::Format::eR32G32B32Sfloat);
	attributeDescriptions[1].setOffset(offsetof(Vertex, color));

	attributeDescriptions[2].setBinding(0);
	attributeDescriptions[2].setLocation(2);
	attributeDescriptions[2].setFormat(vk::Format::eR32G32Sfloat);
	attributeDescriptions[2].setOffset(offsetof(Vertex, texCoord));

	return attributeDescriptions;
}

bool Vertex::operator==(Vertex vert) const
{
	return vert.pos == this->color && vert.color == this->color && vert.texCoord == this->texCoord;
}
#include "stdafx.h"
#include "Buffer.h"
#include "../Rendering/RenderingDevice.h"
#include "../Rendering/VulkanHelpers.h"

using namespace Rendering;
Buffer::Buffer(Type type, Usage usage, std::size_t size):
m_type(type),
m_usage(usage),
m_size(size),
m_buffer()
{
	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	switch (type)
	{
	case Type::Vertex: bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; break;
	case Type::Index: bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT; break;
	case Type::Uniform: bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; break;
	}
	bufferInfo.size = size;

	VmaAllocationCreateInfo allocInfo = {};
	switch (usage)
	{
	case Usage::Static: allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; break;
	case Usage::Mapped: allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;  break;
	}

	VkBuffer buffer;
	auto r = vmaCreateBuffer(m_device->getVMA(), &bufferInfo, &allocInfo, &buffer, &m_allocation, nullptr);
	checkVkResult(r);
	m_buffer = buffer;
}

Buffer::~Buffer()
{
	vmaDestroyBuffer(m_device->getVMA(), m_buffer, m_allocation);
}

Buffer::Type Buffer::getType() const
{
	return m_type;
}

Buffer::Usage Buffer::getUsage() const
{
	return m_usage;
}

void* Buffer::map()
{
	void* data = nullptr;
	VkResult r = vmaMapMemory(m_device->getVMA(), m_allocation, &data);
	checkVkResult(r);
	return data;
}

void Buffer::unmap()
{
	vmaUnmapMemory(m_device->getVMA(), m_allocation);
}

std::size_t Buffer::getSize() const
{
	return m_size;
}
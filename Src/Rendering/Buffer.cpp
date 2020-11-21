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
	recreate(type, usage, size);
}

Buffer::~Buffer()
{
	recreate(m_type, m_usage, 0);
}

void Buffer::recreate(Type type, Usage usage, std::size_t size)
{
	if(m_buffer)
		vmaDestroyBuffer(m_device->getVMA(), m_buffer, m_allocation);

	if (size <= 0)
		return;

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

void Buffer::grow(std::size_t minSize)
{
	if (m_size >= minSize || minSize < 0)
		return;

	recreate(m_type, m_usage, m_size == 0 ? 64 : m_size * 2);
}

void Buffer::setFormat(std::vector<Format>&& format, std::size_t stride)
{
	m_format = std::move(format);
	m_stride = stride;
}

const std::vector<Buffer::Format>& Buffer::getFormat() const
{
	return m_format;
}

std::size_t Buffer::getStride() const
{
	return m_stride;
}

std::size_t Buffer::getSize() const
{
	return m_size;
}

vk::Buffer Buffer::getVkBuffer() const
{
	return m_buffer;
}
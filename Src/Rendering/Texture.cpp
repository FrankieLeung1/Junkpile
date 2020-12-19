#include "stdafx.h"

#include "Texture.h"
#include "../Misc/Misc.h"
#include "../Files/File.h"
#include "../Files/FileManager.h"
#include "../Managers/EventManager.h"
#include "VulkanHelpers.h"
#include "RenderingDevice.h"
#include "Unit.h"

using namespace Rendering;

struct Texture::VulkanImpl
{
	VkImage					m_image = VK_NULL_HANDLE;
	VkImageView				m_view = VK_NULL_HANDLE;

	VmaAllocation			m_memory = VK_NULL_HANDLE;
	VkDeviceSize			m_bufferMemoryAlignment = 256;
};

struct Texture::VulkanImplImgui : public Texture::VulkanImpl
{
	VkDescriptorSetLayout	m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet			m_descriptorSet = VK_NULL_HANDLE;

	VkSampler				m_sampler = VK_NULL_HANDLE;
};

Texture::Texture():
m_mode(Mode::EMPTY),
m_width(-1),
m_height(-1),
m_pixelSize(-1),
m_p(new VulkanImplImgui()),
m_textureData(),
m_samplers(),
m_defaultSampler()
{
	memset(m_p, 0x00, sizeof(VulkanImplImgui));
}

Texture::~Texture()
{
	VmaAllocator vma = m_device->getVMA();
	VkAllocationCallbacks* allocator = m_device->getAllocator();
	setSurfaceImage(vk::Image(), VK_NULL_HANDLE);
	if (m_p->m_view != VK_NULL_HANDLE) vkDestroyImageView(m_device->getDevice(), m_p->m_view, allocator);
	if (m_p->m_descriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_device->getDevice(), m_p->m_descriptorSetLayout, allocator);
	if (m_p->m_sampler != VK_NULL_HANDLE) vkDestroySampler(m_device->getDevice(), m_p->m_sampler, allocator);
	delete m_p;
}

void Texture::createDeviceObjects(Device* device)
{
#if 1
	VkResult err;
	VkAllocationCallbacks* allocator = nullptr;

	if(!m_p->m_sampler)
	{
		VkSamplerCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.magFilter = VK_FILTER_LINEAR;
		info.minFilter = VK_FILTER_LINEAR;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.minLod = -1000;
		info.maxLod = 1000;
		info.maxAnisotropy = 1.0f;
		err = vkCreateSampler(device->getDevice(), &info, allocator, &m_p->m_sampler);
		checkVkResult(err);
	}

	if (!m_p->m_descriptorSetLayout)
	{
		VkSampler sampler[1] = { m_p->m_sampler };
		VkDescriptorSetLayoutBinding binding[1] = {};
		binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding[0].descriptorCount = 1;
		binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		binding[0].pImmutableSamplers = sampler;
		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.bindingCount = 1;
		info.pBindings = binding;
		err = vkCreateDescriptorSetLayout(device->getDevice(), &info, allocator, &m_p->m_descriptorSetLayout);
		checkVkResult(err);
	}

	// Create Descriptor Set:
	if(!m_p->m_descriptorSet)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = device->getPersistentDescriptorPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_p->m_descriptorSetLayout;
		err = vkAllocateDescriptorSets(device->getDevice(), &allocInfo, &m_p->m_descriptorSet);
		checkVkResult(err);
	}
#endif
}

void* Texture::map()
{
	switch (m_mode)
	{
	case Mode::SOFTWARE:
		return &m_textureData.front();

	case Mode::HOST_VISIBLE:
		{
			void* data;
			VkResult r = vmaMapMemory(m_device->getVMA(), m_p->m_memory, &data);
			checkVkResult(r);
			return data;
		}
	}

	return nullptr;
}

void Texture::unmap()
{
	switch (m_mode)
	{
	case Mode::SOFTWARE:
		break;

	case Mode::HOST_VISIBLE:
		vmaUnmapMemory(m_device->getVMA(), m_p->m_memory);
		break;
	}
}

void Texture::flush()
{
	if (m_mode == Mode::HOST_VISIBLE)
		vmaFlushAllocation(m_device->getVMA(), m_p->m_memory, 0, VK_WHOLE_SIZE);
}

Texture::Mode Texture::getMode() const
{
	return m_mode;
}

void Texture::setSoftware(int width, int height, int pixelSize)
{
	CHECK_F(m_mode == Mode::EMPTY);
	CHECK_F(width > 0 && height > 0 && pixelSize > 0);

	m_mode = Mode::SOFTWARE;
	m_width = width;
	m_height = height;
	m_pixelSize = pixelSize;
	m_textureData.resize(width * height * pixelSize);
}

void Texture::setHostVisible(int width, int height, int pixelSize)
{
	CHECK_F(m_mode == Mode::EMPTY);

	m_mode = Mode::HOST_VISIBLE;
	m_width = width;
	m_height = height;
	m_pixelSize = pixelSize;

	ResourcePtr<Rendering::Device> device;
	VkResult err;
	VkAllocationCallbacks* allocator = nullptr;
	VmaAllocator vma = device->getVMA();

	createDeviceObjects(device);

	// Create the Image:
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = m_width;
		info.extent.height = m_height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_LINEAR; // info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		err = vmaCreateImage(vma, &info, &allocInfo, &m_p->m_image, &m_p->m_memory, nullptr);
		checkVkResult(err);
	}

	// Create the Image View:
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_p->m_image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		err = vkCreateImageView(device->getDevice(), &info, allocator, &m_p->m_view);
		checkVkResult(err);
	}

	// Update the Descriptor Set:
	{
		VkDescriptorImageInfo imageDesc[1] = {};
		imageDesc[0].sampler = m_p->m_sampler;
		imageDesc[0].imageView = m_p->m_view;
		imageDesc[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		VkWriteDescriptorSet writeDesc[1] = {};
		writeDesc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDesc[0].dstSet = m_p->m_descriptorSet;
		writeDesc[0].descriptorCount = 1;
		writeDesc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDesc[0].pImageInfo = imageDesc;
		vkUpdateDescriptorSets(device->getDevice(), 1, writeDesc, 0, NULL);
	}

	Rendering::Unit unit = device->createUnit();
	unit.in(ResourcePtr<Rendering::Texture>(NoOwnershipPtr, this));
	unit.in(vk::ImageLayout::eGeneral);
	unit.submit();
}

void Texture::uploadTexture(Device* device, VkCommandBuffer _commandBuffer, Surface* target)
{
	vk::CommandBuffer commandBuffer = static_cast<VkCommandBuffer>(_commandBuffer);

	VkResult err;
	VkAllocationCallbacks* allocator = nullptr;
	VmaAllocator vma = device->getVMA();

	char* pixels = &m_textureData[0];
	size_t uploadSize = m_textureData.size();

	VkImage image = VK_NULL_HANDLE;
	VmaAllocation imageMemory = VK_NULL_HANDLE;
	VkBuffer uploadBuffer = VK_NULL_HANDLE;

	// Create the Image:
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = m_width;
		info.extent.height = m_height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocationCreateInfo allocInfo = {};
		err = vmaCreateImage(vma, &info, &allocInfo, &image, &imageMemory, nullptr);
		checkVkResult(err);
	}

	uploadBuffer = device->createTransferBuffer(uploadSize, pixels);

	// Copy to Image:
	{
		VkImageMemoryBarrier copyBuffer[1] = {};
		copyBuffer[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copyBuffer[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copyBuffer[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copyBuffer[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyBuffer[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copyBuffer[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copyBuffer[0].image = image;
		copyBuffer[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyBuffer[0].subresourceRange.levelCount = 1;
		copyBuffer[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copyBuffer);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = m_width;
		region.imageExtent.height = m_height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(commandBuffer, uploadBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier useBuffer[1] = {};
		useBuffer[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		useBuffer[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		useBuffer[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		useBuffer[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		useBuffer[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		useBuffer[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		useBuffer[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		useBuffer[0].image = image;
		useBuffer[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		useBuffer[0].subresourceRange.levelCount = 1;
		useBuffer[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, useBuffer);
	}

	target->setSurfaceImage(image, imageMemory);
}

void Texture::uploadTexture(Device* device, VkCommandBuffer vcommandBuffer)
{
	createDeviceObjects(device);

#if 1
	VkResult err;
	VkCommandBuffer commandBuffer = static_cast<VkCommandBuffer>(vcommandBuffer);
	VkAllocationCallbacks* allocator = device->getAllocator();
	VmaAllocator vma = device->getVMA();

	char* pixels = &m_textureData[0];
	size_t uploadSize = m_textureData.size();

	// Create the Image:
	VkImage image;
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.extent.width = m_width;
		info.extent.height = m_height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		err = vmaCreateImage(vma, &info, &allocInfo, &image, &m_p->m_memory, nullptr);
		checkVkResult(err);
	}

	// Create the Image View:
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		err = vkCreateImageView(device->getDevice(), &info, allocator, &m_p->m_view);
		checkVkResult(err);
	}

	// Update the Descriptor Set:
	{
		VkDescriptorImageInfo imageDesc[1] = {};
		imageDesc[0].sampler = m_p->m_sampler;
		imageDesc[0].imageView = m_p->m_view;
		imageDesc[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkWriteDescriptorSet writeDesc[1] = {};
		writeDesc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDesc[0].dstSet = m_p->m_descriptorSet;
		writeDesc[0].descriptorCount = 1;
		writeDesc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDesc[0].pImageInfo = imageDesc;
		vkUpdateDescriptorSets(device->getDevice(), 1, writeDesc, 0, NULL);
	}

	VkBuffer uploadBuffer = device->createTransferBuffer(uploadSize, pixels);

	// Copy to Image:
	{
		VkImageMemoryBarrier copyBuffer[1] = {};
		copyBuffer[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		copyBuffer[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copyBuffer[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		copyBuffer[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyBuffer[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copyBuffer[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		copyBuffer[0].image = image;
		copyBuffer[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyBuffer[0].subresourceRange.levelCount = 1;
		copyBuffer[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copyBuffer);

		VkBufferImageCopy region = {};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = m_width;
		region.imageExtent.height = m_height;
		region.imageExtent.depth = 1;
		vkCmdCopyBufferToImage(commandBuffer, uploadBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier useBuffer[1] = {};
		useBuffer[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		useBuffer[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		useBuffer[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		useBuffer[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		useBuffer[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		useBuffer[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		useBuffer[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		useBuffer[0].image = image;
		useBuffer[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		useBuffer[0].subresourceRange.levelCount = 1;
		useBuffer[0].subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, useBuffer);
	}

	m_p->m_image = image;

#endif
}

void Texture::bind(Device* device, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
	if (m_p->m_descriptorSet == VK_NULL_HANDLE)
	{
		ResourcePtr<VulkanFramework> vf;
		vf->uploadTexture(this);
	}

	VkDescriptorSet desc_set[1] = { m_p->m_descriptorSet };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, desc_set, 0, NULL);
}

int Texture::getWidth() const
{
	return m_width;
}

int Texture::getHeight() const
{
	return m_height;
}

int Texture::getPixelSize() const
{
	return m_pixelSize;
}

vk::Image Texture::getVkImage() const
{
	return m_p->m_image;
}

void Texture::setSurfaceImage(vk::Image image, VmaAllocation memory)
{
	if (m_p->m_image != VK_NULL_HANDLE &&  m_p->m_memory != VK_NULL_HANDLE)
		vmaDestroyImage(m_device->getVMA(), m_p->m_image, m_p->m_memory);

	m_p->m_image = image;
	m_p->m_memory = memory;
	m_mode = Mode::DEVICE_LOCAL;
}

vk::Image Texture::getSurfaceImage()
{
	return m_p->m_image;
}

vk::ImageView Texture::getImageView()
{
	if(!m_p->m_view)
	{
		ResourcePtr<Device> device;
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_p->m_image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		auto err = vkCreateImageView(device->getDevice(), &info, nullptr, &m_p->m_view);
		checkVkResult(err);
	}

	return m_p->m_view;
}

vk::ImageLayout Texture::getImageLayout() const
{
	if (m_mode == Mode::DEVICE_LOCAL)
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	else
		return vk::ImageLayout::eGeneral;
}

void Texture::setSampler(std::size_t index, const Unit& unit)
{
	m_samplers.insert(std::make_pair(index, unit));
}

Unit& Texture::getSampler(std::size_t index)
{
	auto it = m_samplers.find(index);
	if (it != m_samplers.end())
		return it->second;

	if (!m_defaultSampler)
	{
		ResourcePtr<Device> device;
		m_defaultSampler = std::make_unique<Unit>(device->getRootUnit());
	}

	return *m_defaultSampler;
}

Texture* Texture::loadPng(const File& file)
{
	unsigned char* pixels;
	unsigned int width, height;
	int pngerror = lodepng_decode32(&pixels, &width, &height, (const unsigned char*)file.getContents().c_str(), file.getSize());
	if (pngerror == 0)
	{
		Rendering::Texture* texture = new Rendering::Texture();
		texture->setSoftware(width, height, 4);
		char* dest = (char*)texture->map();
		memcpy(dest, pixels, width * height * 4);
		texture->unmap();
		free(pixels);

		return texture;
	}
	else
	{
		LOG_F(WARNING, "loadpng failed \"%s\" (%d)", file.getPath().c_str(), pngerror);
		return nullptr;
	}
}

Texture::Loader::Loader():
m_path(),
m_file(EmptyPtr)
{
}

Texture::Loader::Loader(StringView path):
m_path(path.c_str()),
m_file(NewPtr, path)
{
}

Texture::Loader::~Loader()
{
}

Resource* Texture::Loader::load(std::tuple<int, std::string>* error)
{
	if (!ready(error, m_file))
		return nullptr;

	std::string ext = m_path.substr(m_path.find_last_of('.'));
	if (ext == ".png")
	{
		// TODO: use Texture::loadPng() instead
		unsigned char* pixels;
		unsigned int width, height;
		int pngerror = lodepng_decode32(&pixels, &width, &height, (const unsigned char*)m_file->getContents().c_str(), m_file->getSize());
		if (pngerror == 0)
		{
			Rendering::Texture* texture = new Rendering::Texture();
			texture->setSoftware(width, height, 4);
			char* dest = (char*)texture->map();
			memcpy(dest, pixels, width * height * 4);
			texture->unmap();
			free(pixels);

			return texture;
		}
		else
		{
			*error = { -1, stringf("loadpng failed \"%s\" (%d)", m_path.c_str(), pngerror) };
		}
	}
	else
	{
		*error = { -1, stringf("Unknown texture format \"%s\"", m_path) };
	}

	return nullptr;
}

Texture::Reloader* Texture::Loader::createReloader()
{
	std::string path = m_path;
	return new ReloaderOnFileChange(path, [path]() { return new Loader(path); });
}

std::string Texture::Loader::getDebugName() const
{
	return m_path;
}

StringView Texture::Loader::getTypeName() const
{
	return "Texture";
}

Texture::Loader* Texture::createLoader(StringView path)
{
	return new Loader(path);
}

std::tuple<bool, std::size_t> Texture::getSharedHash()
{
	return { false, 0 };
}

std::tuple<bool, std::size_t> Texture::getSharedHash(StringView path)
{
	return { true, std::hash<const char*>{}(path.c_str()) };
}

Resource* Texture::getSingleton()
{
	return nullptr;
}

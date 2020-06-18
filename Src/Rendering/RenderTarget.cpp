#include "stdafx.h"
#include "RenderTarget.h"
#include "RenderingDevice.h"
#include "VulkanHelpers.h"
using namespace Rendering;

RenderTarget::RenderTarget(Type type, int width, int height):
RenderTarget(getFormat(type), getTiling(type), getFeatures(type), width, height)
{

}

RenderTarget::RenderTarget(vk::Format format, vk::ImageTiling tiling, vk::FormatFeatureFlags features, int width, int height):
m_format(format)
{
    CHECK_F(format != vk::Format::eUndefined);

    ResourcePtr<Rendering::Device> device;
    VkResult err;
    VkAllocationCallbacks* allocator = nullptr;
    VmaAllocator vma = device->getVMA();

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    bool isDepthStencil = (features == vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    if(isDepthStencil)
        imageInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    err = vmaCreateImage(vma, (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&m_image, &m_allocation, nullptr);
    checkVkResult(err);
    
    vk::ImageViewCreateInfo viewInfo = {};
    viewInfo.image = m_image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    if (isDepthStencil)
    {
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if(hasStencil(m_format))
            viewInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
    else
    {
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    m_view = device->createObject(viewInfo);
}

RenderTarget::RenderTarget(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, int width, int height):
    RenderTarget(findSupportedFormat(candidates, tiling, features), tiling, features, width, height) {}

RenderTarget::~RenderTarget()
{
    ResourcePtr<Rendering::Device> device;
    if (m_view) device->destroyObject(m_view);
    if (m_image) vmaDestroyImage(device->getVMA(), m_image, m_allocation);
}

vk::Format RenderTarget::getFormat() const
{
    return m_format;
}

vk::Image RenderTarget::getImage()
{
    return m_image;
}

vk::ImageView RenderTarget::getView()
{
    return m_view;
}

vk::Format RenderTarget::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const
{
    ResourcePtr<Rendering::Device> device;
    vk::PhysicalDevice physicalDevice = device->getPhysicalDevice();
    for (vk::Format format : candidates) {
        vk::FormatProperties props;
        physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) return format;
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) return format;
    }

    return vk::Format::eUndefined;
}

bool RenderTarget::hasStencil(vk::Format format) const
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

std::vector<vk::Format> RenderTarget::getFormat(Type type) const
{
    switch (type)
    {
    case Type::DepthStencil: return { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat };
    default: LOG_F(FATAL, ""); return {};
    };
}

vk::ImageTiling RenderTarget::getTiling(Type type) const
{
    switch (type)
    {
    case Type::DepthStencil: return vk::ImageTiling::eOptimal;
    default: LOG_F(FATAL, ""); return vk::ImageTiling(); // invalid
    };
}

vk::FormatFeatureFlags RenderTarget::getFeatures(Type type) const
{
    switch (type)
    {
    case Type::DepthStencil: return vk::FormatFeatureFlagBits::eDepthStencilAttachment;
    default: LOG_F(FATAL, ""); return vk::FormatFeatureFlags{};
    };
}
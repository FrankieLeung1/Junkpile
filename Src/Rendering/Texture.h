#pragma once

#include "RenderTarget.h"
#include "DataSource.h"
typedef void* ImTextureID;
namespace Rendering
{
	class Device;
	class Texture : public Resource, public DataSource, public RenderTarget
	{
	public:
		enum class Mode {
			EMPTY,
			SOFTWARE,
			HOST_VISIBLE, // mappable
			DEVICE_LOCAL
		};

	public:
		Texture();
		~Texture();

		void* map();
		void flush();
		void unmap();

		Mode getMode() const;
		void setSoftware(int width, int height, int pixelSize);
		void setHostVisible(int width, int height, int pixelSize);

		void uploadTexture(Device* device, VkCommandBuffer commandBuffer);
		void uploadTexture(Device* device, VkCommandBuffer commandBuffer, RenderTarget*);
		void bind(Device* device, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		int getWidth() const;
		int getHeight() const;
		int getPixelSize() const;
		vk::Image getVkImage() const;

		void setVkImageRT(vk::Image image, VmaAllocation memory);
		vk::Image getVkImageRT();

	//protected:
		void createDeviceObjects(Device*);

	protected:
		Mode m_mode;
		int m_width, m_height, m_pixelSize;

		std::vector<char> m_textureData;
		ResourcePtr<Rendering::Device> m_device;

		struct VulkanImpl;
		struct VulkanImplImgui;
		VulkanImplImgui* m_p;

		friend class Unit;
	};
}
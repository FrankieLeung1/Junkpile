#pragma once

#include "Surface.h"
#include "DataSource.h"
typedef void* ImTextureID;
class File;
namespace Rendering
{
	class Device;
	class Unit;
	class Texture : public Resource, public DataSource, public Surface
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
		void uploadTexture(Device* device, VkCommandBuffer commandBuffer, Surface*);
		void bind(Device* device, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		int getWidth() const;
		int getHeight() const;
		int getPixelSize() const;
		vk::Image getVkImage() const;

		void setSurfaceImage(vk::Image image, VmaAllocation memory);
		vk::Image getSurfaceImage();

		vk::ImageView getImageView();
		vk::ImageLayout getImageLayout() const;

		void setSampler(std::size_t index, const Unit&);
		Unit& getSampler(std::size_t index);

	//protected:
		void createDeviceObjects(Device*);

	protected:
		Mode m_mode;
		int m_width, m_height, m_pixelSize;

		std::vector<char> m_textureData;
		ResourcePtr<Rendering::Device> m_device;
		std::map<std::size_t, Unit> m_samplers;
		std::unique_ptr<Unit> m_defaultSampler;

		struct VulkanImpl;
		struct VulkanImplImgui;
		VulkanImplImgui* m_p;

		friend class Unit;

	public:
		class Loader : public Resource::Loader
		{
		public:
			enum Error {
				FileNotFound = 1
			};

		public:
			Loader();
			Loader(StringView path);
			~Loader() override;
			Resource* load(std::tuple<int, std::string>* error) override;
			Loader* createReloader() override;
			std::string getDebugName() const override;
			StringView getTypeName() const override;

		protected:
			std::string m_path;
			ResourcePtr<File> m_file;
		};

		static Loader* createLoader(StringView path);
		static Resource* getSingleton();

		static std::tuple<bool, std::size_t> getSharedHash();
		static std::tuple<bool, std::size_t> getSharedHash(StringView path);
	};
}
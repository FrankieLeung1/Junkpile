#pragma once

#include "../Misc/Any.h"
#include "../Resources/ResourceManager.h"
#include "../Misc/Callbacks.h"
namespace Rendering
{
	class Unit;
	class RootUnit;
	class Shader;
	class Device : public SingletonResource<Device>
	{
	public:
		Device();
		~Device();

		vk::Instance getInstance() const;
		vk::PhysicalDevice getPhysicalDevice() const;
		vk::Device getDevice() const;
		vk::Queue getQueue() const;

		void setInstance(vk::Instance);
		void setPhysicalDevice(vk::PhysicalDevice);
		void setDevice(vk::Device);
		void setQueue(vk::Queue);
		void setPipelineCache(vk::PipelineCache);
		void setRenderPass(vk::RenderPass);
		void setFrameBufferDimensions(void* window, const glm::vec2& dimensions);
		void setFrameBuffers(void* window, const std::vector<vk::Framebuffer>&);
		void setFrameFences(void* window, const std::vector<vk::Fence>& fences);
		void setCurrentWindow(void* window);
		void setCurrentFrame(std::size_t index);

		void createRenderPass(vk::Format);

		vk::RenderPass getRenderPass() const;
		std::tuple<vk::Framebuffer, glm::vec2> getFrameBuffer() const;
		vk::DescriptorPool getDescriptorPool(const std::thread::id & = std::this_thread::get_id());
		vk::DescriptorPool getPersistentDescriptorPool(const std::thread::id & = std::this_thread::get_id());

		VmaAllocator getVMA() const;
		VkAllocationCallbacks* getAllocator() const;

		RootUnit& getRootUnit();
		Unit createUnit();
		void submitAll();

		void update();

		template<typename Object>
		void setDebugName(Object, const char*) { LOG_F(INFO, "todo: setDebugName\n"); }

		void beginDebugRegion(VkCommandBuffer, const char*);
		void endDebugRegion(VkCommandBuffer);

		VkBuffer createTransferBuffer(std::size_t, void* data = nullptr);
		vk::Sampler createObject(const vk::SamplerCreateInfo&);
		vk::DescriptorSetLayout createObject(const vk::DescriptorSetLayoutCreateInfo&);
		vk::Pipeline createObject(const vk::GraphicsPipelineCreateInfo&);
		vk::PipelineLayout createObject(const vk::PipelineLayoutCreateInfo&);
		vk::RenderPass createObject(const vk::RenderPassCreateInfo&);
		vk::ShaderModule createObject(const vk::ShaderModuleCreateInfo&);
		template<typename T> T& getObject(std::size_t id) const;

		void destroyObject(vk::RenderPass);

		vk::DescriptorSet allocateObject(const vk::DescriptorSetAllocateInfo&);

		void updateObject(const std::vector<vk::WriteDescriptorSet>&);

		void imgui();

		void allocateThreadResources(const std::thread::id&);

		void waitIdle();

	public:
		static std::tuple<bool, std::size_t> getSharedHash();
		static Loader* createLoader(); // not used since this is a singleton resource

	protected:
		vk::Result create(vk::DescriptorPool*) const;
		vk::Result create(vk::CommandPool*) const;

		struct ThreadResources;
		//void allocateThreadResources(const std::thread::id&);
		ThreadResources* getThreadResources();

	protected:
		std::mutex m_mutex;

		vk::Instance m_instance;
		vk::PhysicalDevice m_physicalDevice;
		vk::Device m_device;
		vk::Queue m_queue;
		vk::AllocationCallbacks* m_allocator{ nullptr };
		vk::PipelineCache m_pipelineCache;
		vk::RenderPass m_renderPass;
		VmaAllocator m_vma;
		Fossilize::StateRecorder m_recorder;

		struct ThreadResources
		{
			vk::CommandPool m_commandPool;
			vk::CommandBuffer m_commandBuffers[12];
			std::size_t m_commandBuffersUsed{ 0 };

			std::vector< std::tuple<VkBuffer, VmaAllocation> > m_transferBuffers;
			vk::DescriptorPool m_descriptorPool;
			vk::DescriptorPool m_persistentDescriptorPool;
		};

		struct FrameResources
		{
			vk::Framebuffer m_frameBuffer;
			vk::Fence m_fence;
			std::map<std::thread::id, ThreadResources> m_threadResources;
		};

		struct WindowResources
		{
			glm::vec2 m_frameDimensions;
			std::vector<FrameResources*> m_frameResources;
		};
		std::map<void*, WindowResources> m_windowResources;
		FrameResources* m_currentFrameResources;
		WindowResources* m_currentWindowResources;

		int m_currentFrame{ 0 };

		RootUnit* m_rootUnit;

		// imgui
		int m_selectedUnit;
		int m_selectedInOut;

		std::map<Fossilize::Hash, Any> m_objects;

		friend class VulkanFramework;
		friend class Unit;
	};

	// ----------------------- IMPLEMENTATION -----------------------
	template<typename T>
	T& Device::getObject(std::size_t id) const
	{
		auto& it = m_objects.find(id);
		CHECK_F(it->second.isType<T>());
		return it->second.get<T>();
	}
}
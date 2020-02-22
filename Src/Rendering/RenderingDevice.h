#pragma once

#include "../Misc/Any.h"
#include "../Resources/ResourceManager.h"
#include "../Misc/Callbacks.h"
namespace Rendering
{
	class Unit;
	class RootUnit;
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

		vk::DescriptorPool getDescriptorPool(const std::thread::id& = std::this_thread::get_id());

		VmaAllocator getVMA() const;
		VkAllocationCallbacks* getAllocator() const;

		Unit& getRootUnit();
		Unit createUnit();
		void submitAll();

		void waitFor(vk::Fence, FunctionBase<void>*);

		void update();

		template<typename Object>
		void setDebugName(Object, const char*) { LOG_F(INFO, "todo: setDebugName\n"); }

		void beginDebugRegion(VkCommandBuffer, const char*);
		void endDebugRegion(VkCommandBuffer);

		VkBuffer createTransferBuffer(std::size_t, void* data = nullptr);
		vk::Sampler* createObject(const vk::SamplerCreateInfo&);
		template<typename T> T& getObject(std::size_t id) const;

		void imgui();

		void allocateThreadResources(const std::thread::id&);

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

		ResourcePtr<Device> m_resourceHandle;

		vk::Instance m_instance;
		vk::PhysicalDevice m_physicalDevice;
		vk::Device m_device;
		vk::Queue m_queue;
		vk::AllocationCallbacks* m_allocator{ nullptr };
		vk::PipelineCache m_pipelineCache;
		VmaAllocator m_vma;

		std::map<std::size_t, Any> m_objects;

		struct FenceCallbacks
		{
			vk::Fence m_fence;
			FunctionBase<void>* m_callback;
		};
		std::vector<FenceCallbacks> m_fenceCallbacks;

		struct ThreadResources
		{
			vk::CommandPool m_commandPool;
			vk::CommandBuffer m_commandBuffers[12];
			std::size_t m_commandBuffersUsed{ 0 };

			std::vector< std::tuple<VkBuffer, VmaAllocation> > m_transferBuffers;
			vk::DescriptorPool m_descriptorPool;
		};
		std::map<std::thread::id, ThreadResources> m_threadResources;

		RootUnit* m_rootUnit;

		// imgui
		int m_selectedUnit;
		int m_selectedInOut;

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
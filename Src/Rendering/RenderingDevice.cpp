#include "stdafx.h"

#include "imgui/imgui.h"
#include "RenderingDevice.h"
#include "VulkanHelpers.h"
#include "../imgui/ImGuiManager.h"
#include "../Managers/InputManager.h"
#include "../Misc/Misc.h"
#include "../Managers/EventManager.h"
#include "Unit.h"

using namespace Rendering;

Device::Device():
m_instance(),
m_physicalDevice(),
m_device(),
m_queue(),
m_resourceHandle(EmptyPtr),
m_pipelineCache(),
m_renderPass(),
m_vma(nullptr),
m_rootUnit(new RootUnit()),
m_selectedUnit(-1),
m_selectedInOut(-1)
{
	ResourcePtr<EventManager> events;
	events->addListener<UpdateEvent>([this](const UpdateEvent*) { this->update(); return EventManager::ListenerResult::Persist; }, 11);
	events->addListener<UpdateEvent>([this](const UpdateEvent*) { this->submitAll(); return EventManager::ListenerResult::Persist; }, -10);
	
}

Device::~Device()
{
	delete m_rootUnit;

	for (auto& value : m_objects)
	{
		Any& any = value.second;
		if (auto v = any.getPtr<vk::ShaderModule>()) m_device.destroyShaderModule(*v);
		else if (auto v = any.getPtr<vk::RenderPass>()) m_device.destroyRenderPass(*v);
		else if (auto v = any.getPtr<vk::PipelineLayout>()) m_device.destroyPipelineLayout(*v);
		else if (auto v = any.getPtr<vk::Pipeline>()) m_device.destroyPipeline(*v);
		else if (auto v = any.getPtr<vk::DescriptorSetLayout>()) m_device.destroyDescriptorSetLayout(*v);
		else if (auto v = any.getPtr<vk::Sampler>()) m_device.destroySampler(*v);
	}

	for (auto& threadResources : m_threadResources)
	{
		for(auto& transfer : threadResources.second.m_transferBuffers)
			vmaDestroyBuffer(m_vma, std::get<VkBuffer>(transfer), std::get<VmaAllocation>(transfer));

		
		m_device.destroyDescriptorPool(threadResources.second.m_persistentDescriptorPool);
		m_device.destroyDescriptorPool(threadResources.second.m_descriptorPool);
		m_device.destroyCommandPool(threadResources.second.m_commandPool);
	}

	vmaDestroyAllocator(m_vma);
	m_device.destroy(m_allocator);
	m_instance.destroy(m_allocator);
}

vk::Instance Device::getInstance() const
{
	return m_instance;
}

vk::PhysicalDevice Device::getPhysicalDevice() const
{
	return m_physicalDevice;
}

vk::Device Device::getDevice() const
{
	return m_device; 
}

vk::Queue Device::getQueue() const
{
	return m_queue;
}

vk::DescriptorPool Device::getDescriptorPool(const std::thread::id& id)
{
	std::lock_guard<std::mutex> l(m_mutex);
	auto it = m_threadResources.find(id);
	if (it != m_threadResources.end())
	{
		return it->second.m_descriptorPool;
	}
	
	return nullptr;
}

vk::DescriptorPool Device::getPersistentDescriptorPool(const std::thread::id& id)
{
	std::lock_guard<std::mutex> l(m_mutex);
	auto it = m_threadResources.find(id);
	if (it != m_threadResources.end())
	{
		return it->second.m_persistentDescriptorPool;
	}

	return nullptr;
}

VmaAllocator Device::getVMA() const
{
	return m_vma;
}

VkAllocationCallbacks* Device::getAllocator() const
{
	return nullptr;
}

void Device::setInstance(vk::Instance instance)
{
	m_instance = instance;
}

void Device::setPhysicalDevice(vk::PhysicalDevice physicalDevice)
{
	m_physicalDevice = physicalDevice;
}

void Device::setDevice(vk::Device device)
{
	m_device = device;
	if (m_vma)
		vmaDestroyAllocator(m_vma);

	VmaAllocatorCreateInfo info = { 0 };
	info.instance = m_instance;
	info.physicalDevice = m_physicalDevice;
	info.device = m_device;
	//info.vulkanApiVersion = VK_API_VERSION_1_1;
	//info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	VkResult r = vmaCreateAllocator(&info, &m_vma);
	checkVkResult(r);
}

void Device::setQueue(vk::Queue queue)
{
	m_queue = queue;
}

void Device::setPipelineCache(vk::PipelineCache cache)
{
	m_pipelineCache = cache;
}

void Device::setRenderPass(vk::RenderPass renderPass)
{
	// TODO: destroy current render pass?
	m_renderPass = renderPass;
}

void Device::setFrameBuffer(vk::Framebuffer frameBuffer, const glm::vec2& d)
{
	m_frameBuffer = frameBuffer;
	m_frameDimensions = d;
}

void Device::createRenderPass(vk::Format format)
{
	VkAttachmentDescription attachment = {};
	attachment.format = (VkFormat)format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	setRenderPass(createObject(info));
}

vk::RenderPass Device::getRenderPass() const
{
	return m_renderPass;
}

std::tuple<vk::Framebuffer, glm::vec2> Device::getFrameBuffer() const
{
	return std::tie(m_frameBuffer, m_frameDimensions);
}

RootUnit& Device::getRootUnit()
{
	return *m_rootUnit;
}

Unit Device::createUnit()
{
	return Unit(*m_rootUnit);
}

void Device::submitAll()
{
	if (m_rootUnit->m_submitted.empty())
		return; 
	auto it = m_threadResources.find(std::this_thread::get_id());
	if (it == m_threadResources.end())
		return;

	ThreadResources& resources = it->second;
	LOG_IF_F(FATAL, resources.m_commandBuffersUsed >= countof(resources.m_commandBuffers), "Ran out of command buffers");
	vk::CommandBuffer commandBuffer = resources.m_commandBuffers[resources.m_commandBuffersUsed++];

	vk::CommandBufferBeginInfo beginInfo = {};
	beginInfo.flags |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	vk::Result r = commandBuffer.begin(beginInfo);
	checkVkResult(r);

	for (auto& unit : m_rootUnit->m_submitted)
	{
		unit.submitCommandBuffers(this, &resources, commandBuffer);
	}

	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;
	m_queue.submit(1, &submitInfo, vk::Fence());

	m_rootUnit->m_submitted.clear();
	resources.m_commandBuffersUsed = 0;
}

void Device::update()
{
	m_device.waitIdle();

	ThreadResources* threadRes = getThreadResources();
	m_device.resetDescriptorPool(threadRes->m_descriptorPool);
}

VkBuffer Device::createTransferBuffer(std::size_t size, void* data)
{
	VkResult err;
	ThreadResources* resources = getThreadResources();
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	err = vmaCreateBuffer(m_vma, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	checkVkResult(err);

	char* map = NULL;
	err = vmaMapMemory(m_vma, allocation, (void**)(&map));
	checkVkResult(err);
	memcpy(map, data, size);
	vmaFlushAllocation(m_vma, allocation, 0, size);
	checkVkResult(err);
	vmaUnmapMemory(m_vma, allocation);

	resources->m_transferBuffers.push_back(std::tie(buffer, allocation));
	return buffer;
}

vk::Sampler Device::createObject(const vk::SamplerCreateInfo& info)
{
	std::lock_guard<std::mutex> l(m_mutex);
	Fossilize::Hash hash;
	LOG_IF_F(ERROR, !Fossilize::Hashing::compute_hash_sampler(info, &hash),
		"Fossilize::Hashing::compute_hash_sampler failed\n");

	auto it = m_objects.find(hash);
	if (it == m_objects.end())
	{
		vk::Result err;
		vk::Sampler sampler;
		err = m_device.createSampler(&info, m_allocator, &sampler);
		checkVkResult(err);
		m_recorder.record_sampler(sampler, info);

		it = m_objects.insert({hash, sampler}).first;
	}

	return it->second.get<vk::Sampler>();
}

vk::DescriptorSetLayout Device::createObject(const vk::DescriptorSetLayoutCreateInfo& info)
{
	std::lock_guard<std::mutex> l(m_mutex);
	Fossilize::Hash hash;
	LOG_IF_F(ERROR, !Fossilize::Hashing::compute_hash_descriptor_set_layout(m_recorder, info, &hash),
		"Fossilize::Hashing::compute_hash_descriptor_set_layout failed\n");

	auto it = m_objects.find(hash);
	if (it == m_objects.end())
	{
		vk::Result err;
		vk::DescriptorSetLayout descriptorSetLayout;
		err = m_device.createDescriptorSetLayout(&info, m_allocator, &descriptorSetLayout);
		checkVkResult(err);
		m_recorder.record_descriptor_set_layout(descriptorSetLayout, info);

		it = m_objects.insert({ hash, descriptorSetLayout }).first;
	}

	return it->second.get<vk::DescriptorSetLayout>();
}

vk::Pipeline Device::createObject(const vk::GraphicsPipelineCreateInfo& info)
{
	std::lock_guard<std::mutex> l(m_mutex);
	Fossilize::Hash hash;
	LOG_IF_F(ERROR, !Fossilize::Hashing::compute_hash_graphics_pipeline(m_recorder, info, &hash),
		"Fossilize::Hashing::compute_hash_pipeline_layout failed\n");

	auto it = m_objects.find(hash);
	if (it == m_objects.end())
	{
		vk::ResultValue<vk::Pipeline> r = m_device.createGraphicsPipeline(m_pipelineCache, info, m_allocator);
		checkVkResult(r.result);
		m_recorder.record_graphics_pipeline(r.value, info, nullptr, 0);

		it = m_objects.insert({ hash, r.value }).first;
	}

	return it->second.get<vk::Pipeline>();
}

vk::PipelineLayout Device::createObject(const vk::PipelineLayoutCreateInfo& info)
{
	std::lock_guard<std::mutex> l(m_mutex);
	Fossilize::Hash hash;
	LOG_IF_F(ERROR, !Fossilize::Hashing::compute_hash_pipeline_layout(m_recorder, info, &hash),
		"Fossilize::Hashing::compute_hash_pipeline_layout failed\n");

	auto it = m_objects.find(hash);
	if (it == m_objects.end())
	{
		vk::ResultValue<vk::PipelineLayout> r = m_device.createPipelineLayout(info, m_allocator);
		checkVkResult(r.result);
		m_recorder.record_pipeline_layout(r.value, info);

		it = m_objects.insert({ hash, r.value }).first;
	}

	return it->second.get<vk::PipelineLayout>();
}

vk::RenderPass Device::createObject(const vk::RenderPassCreateInfo& info)
{
	std::lock_guard<std::mutex> l(m_mutex);
	Fossilize::Hash hash;
	LOG_IF_F(ERROR, !Fossilize::Hashing::compute_hash_render_pass(info, &hash),
		"Fossilize::Hashing::compute_hash_render_pass failed\n");

	auto it = m_objects.find(hash);
	if (it == m_objects.end())
	{
		vk::ResultValue<vk::RenderPass> r = m_device.createRenderPass(info, m_allocator);
		checkVkResult(r.result);
		m_recorder.record_render_pass(r.value, info);

		it = m_objects.insert({ hash, r.value }).first;
	}

	return it->second.get<vk::RenderPass>();
}

vk::ShaderModule Device::createObject(const vk::ShaderModuleCreateInfo& info)
{
	std::lock_guard<std::mutex> l(m_mutex);
	Fossilize::Hash hash;
	LOG_IF_F(ERROR, !Fossilize::Hashing::compute_hash_shader_module(info, &hash),
		"Fossilize::Hashing::compute_hash_shader_module failed\n");

	auto it = m_objects.find(hash);
	if (it == m_objects.end())
	{
		vk::ResultValue<vk::ShaderModule> r = m_device.createShaderModule(info, m_allocator);
		checkVkResult(r.result);
		m_recorder.record_shader_module(r.value, info);

		it = m_objects.insert({ hash, r.value }).first;
	}

	return it->second.get<vk::ShaderModule>();
}

vk::DescriptorSet Device::allocateObject(const vk::DescriptorSetAllocateInfo& info)
{
	LOG_IF_F(ERROR, info.descriptorSetCount != 1, "descriptorSetCount must be 1\n");
	vk::ResultValue<std::vector<vk::DescriptorSet>> r = m_device.allocateDescriptorSets(info);
	checkVkResult(r.result);
	return r.value.front();
}

void Device::updateObject(const std::vector<vk::WriteDescriptorSet>& info)
{
	m_device.updateDescriptorSets((uint32_t)info.size(), &info[0], 0, nullptr);
}

void Device::destroyObject(vk::RenderPass pass)
{
	for (auto& it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		auto* rp = it->second.getPtr<vk::RenderPass>();
		if (rp  && *rp == pass)
		{
			m_device.destroyRenderPass(*rp);
			m_objects.erase(it);
			return;
		}
	}
}

void Device::imgui()
{
	using namespace ImGui;
	ResourcePtr<ImGuiManager> imgui;
	ResourcePtr<InputManager> input;
	bool* unitsOpened = imgui->win("Units", "Rendering");
	if(*unitsOpened)
	{
		if (Begin("Units", unitsOpened))
		{
			Columns(3);
			Separator();

			std::vector<Unit>& submitted = m_rootUnit->getSubmitted();
			for (int i = 0; i < submitted.size(); ++i)
			{
				Unit& unit = submitted[i];
				if (Selectable(stringf("Draw %d", i).c_str(), m_selectedUnit == i))
				{
					if (m_selectedUnit != i)
					{
						m_selectedInOut = -1;
						m_selectedUnit = i;
					}
				}
			}
			NextColumn();

			if (m_selectedUnit >= 0)
			{
				Unit& unit = submitted[m_selectedUnit];
				if (TreeNodeEx("OUTPUTS", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick))
				{
					ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
					for (int i = 0; i < unit.m_data->m_targets.size(); i++)
					{
						if (Selectable(stringf("Target %d", i).c_str(), m_selectedInOut == i))
						{
							m_selectedInOut = i;
						}
					}
					ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
					TreePop();
				}

				if (TreeNodeEx("INPUTS", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_OpenOnDoubleClick))
				{
					ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
					for (int i = 0; i < unit.m_data->m_settings.size(); i++)
					{
						std::size_t selectedIndex = unit.m_data->m_targets.size() + i;
						if (Selectable(stringf("Setting %d", i).c_str(), m_selectedInOut == selectedIndex))
						{
							m_selectedInOut = (int)selectedIndex;
						}
					}
					ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
					TreePop();
				}
				NextColumn();

	#define X(name) if(selectedAny.isType<name>()) Text(#name);
				if (m_selectedInOut >= 0 && m_selectedInOut < (int)unit.m_data->m_targets.size())
				{
					Text("RenderTarget");
				}
				else if (m_selectedInOut >= 0)
				{
					Any& selectedAny = unit.m_data->m_settings[m_selectedInOut - unit.m_data->m_targets.size()];
					X(Unit);
					X(ResourcePtr<Shader>);
					X(ResourcePtr<Texture>);
					X(ResourcePtr<TextureAtlas>);
					X(vk::SamplerMipmapMode);
					X(vk::CompareOp);
					X(Unit::Named<bool>);
					X(Unit::Named<float>);
					X(Unit::Named<vk::Filter>);
					X(Unit::Named<vk::SamplerAddressMode>);
				}
	#undef X
			}
			Columns(1);
			Separator();
		}
		End();
	}

	bool* memory = imgui->win("Memory", "Rendering");
	if (*memory)
	{
		if (Begin("Memory", memory))
		{
			VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
			for (int i = 0; i < VK_MAX_MEMORY_HEAPS; ++i)
				budgets[i].budget = 0;
			vmaGetBudget(m_vma, budgets);

			const VkPhysicalDeviceMemoryProperties* prop;
			vmaGetMemoryProperties(m_vma, &prop);

			for (int i = 0; i < countof(budgets); ++i)
			{
				if (!budgets[i].budget)
					continue;

				if (i > 0) ImGui::Separator();
				int type = prop->memoryHeaps[i].flags;
				std::stringstream typeStr;
				if (type & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) typeStr << "DEVICE_LOCAL_BIT;";
				if (type & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) typeStr << "HOST_VISIBLE_BIT;";
				if (type & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) typeStr << "HOST_COHERENT_BIT;";
				if (type & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) typeStr << "HOST_CACHED_BIT;";
				if (type & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) typeStr << "LAZILY_ALLOCATED_BIT;";
				if (type & VK_MEMORY_PROPERTY_PROTECTED_BIT) typeStr << "PROTECTED_BIT;";
				if (type & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) typeStr << "DEVICE_COHERENT_BIT_AMD;";
				if (type & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) typeStr << "DEVICE_UNCACHED_BIT_AMD;";
				if (!type) typeStr << "NONE;";
					 
				ImGui::Text("Type: %s", typeStr.str().c_str());
				ImGui::Text("Blocks: %s", prettySize(budgets[i].blockBytes).c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sum size of all VkDeviceMemory blocks allocated");
				ImGui::Text("Allocations: %s", prettySize(budgets[i].allocationBytes).c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sum size of all allocations created in particular heap.\nUsually less or equal than blockBytes. Difference blockBytes - allocationBytes is the amount of memory allocated but unused - available for new allocations or wasted due to fragmentation.\nIt might be greater than blockBytes if there are some allocations in lost state, as they account to this value as well.");
				ImGui::Text("Usage: %s", prettySize(budgets[i].usage).c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Estimated current memory usage of the program.");
				ImGui::Text("Budget: %s", prettySize(budgets[i].budget).c_str());
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Estimated amount of memory available to the program.");
			}
		}
		End();
	}
}

vk::Result Device::create(vk::DescriptorPool* pool) const
{
	CHECK_F(pool != nullptr);

	// Create Descriptor Pool
	using vk::DescriptorType;
	vk::DescriptorPoolSize poolSizes[] =
	{
		{ DescriptorType::eSampler, 1000 },
		{ DescriptorType::eCombinedImageSampler, 1000 },
		{ DescriptorType::eSampledImage, 1000 },
		{ DescriptorType::eStorageImage, 1000 },
		{ DescriptorType::eUniformTexelBuffer, 1000 },
		{ DescriptorType::eStorageTexelBuffer, 1000 },
		{ DescriptorType::eUniformBuffer, 1000 },
		{ DescriptorType::eStorageBuffer, 1000 },
		{ DescriptorType::eUniformBufferDynamic, 1000 },
		{ DescriptorType::eStorageBufferDynamic, 1000 },
		{ DescriptorType::eInputAttachment, 1000 }
	};
	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
	poolInfo.setMaxSets(1000 * (uint32_t)countof(poolSizes));
	poolInfo.setPoolSizeCount((uint32_t)countof(poolSizes));
	poolInfo.setPPoolSizes(poolSizes);
	
	vk::Result r;
	std::tie(r, *pool) = m_device.createDescriptorPool(poolInfo, m_allocator);
	return r;
}

vk::Result Device::create(vk::CommandPool* pool) const
{
	vk::Result r;
	vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	std::tie(r, *pool) = m_device.createCommandPool(commandPoolInfo);
	return r;
}

void Device::allocateThreadResources(const std::thread::id& id)
{
	if (m_threadResources.find(id) != m_threadResources.end())
		return;

	vk::Result r;
	ThreadResources resources;
	r = create(&resources.m_commandPool);
	checkVkResult(r);

	auto buffers = m_device.allocateCommandBuffers({ resources.m_commandPool, vk::CommandBufferLevel::ePrimary, (uint32_t)countof(resources.m_commandBuffers) });
	checkVkResult(buffers.result);
	memcpy(resources.m_commandBuffers, &buffers.value[0], buffers.value.size() * sizeof(vk::CommandBuffer));

	r = create(&resources.m_descriptorPool);
	checkVkResult(r);

	r = create(&resources.m_persistentDescriptorPool);
	checkVkResult(r);

	m_threadResources[id] = resources;
}

void Device::beginDebugRegion(VkCommandBuffer, const char*)
{
	LOG_F(INFO, "todo: beginDebugRegion\n");
}

void Device::endDebugRegion(VkCommandBuffer)
{
	LOG_F(INFO, "todo: endDebugRegion\n");
}

Device::ThreadResources* Device::getThreadResources()
{
	auto it = m_threadResources.find(std::this_thread::get_id());
	return it == m_threadResources.end() ? nullptr : &(it->second);
}

std::tuple<bool, std::size_t> Device::getSharedHash()
{
	const char* t = "RenderingDevice";
	return std::make_tuple(true, reinterpret_cast<std::size_t>("RenderingDevice"));
}

Device::Loader* Device::createLoader()
{
	return nullptr;
}

#include "stdafx.h"
#include "Unit.h"
#include "../Misc/Misc.h"
#include "VulkanHelpers.h"
#include "Buffer.h"
#include "Texture.h"
#include "TextureAtlas.h"
#include "RenderingDevice.h"
#include "Shader.h"
#include "../Resources/ResourceManager.h"

using namespace Rendering;
Unit::Unit() :
m_data(std::make_shared<Data>()),
m_submitted(false)
{
	ResourcePtr<Device> device;
	m_data->m_root = &device->getRootUnit();
}

Unit::Unit(RootUnit& root):
m_data(std::make_shared<Data>()),
m_submitted(false)
{
	m_data->m_root = &root;
}

Unit::Unit(const Unit& unit):
m_data(unit.m_data),
m_submitted(false)
{
}

Unit::~Unit()
{
}

void Unit::submit()
{
	m_data->m_root->m_submitted.push_back(*this);
	m_submitted = true;
}

bool Unit::isEmpty() const
{
	return m_data->m_empty;
}

Unit& Unit::operator=(const Unit& unit)
{
	m_data = unit.m_data;
	return *this;
}

bool Unit::operator==(const Unit& unit) const
{
	return m_data == unit.m_data;
}

Unit::Data& Unit::getData()
{
	if (!m_data)
		m_data = std::make_unique<Data>();

	return *m_data;
}

template <typename Ptr, typename ResPtr>
static bool getResource(Ptr*& ptr, ResPtr* res)
{
	if (!res)
		return false;

	ptr = res->get();
	return (ptr != nullptr);
};

void Unit::submitCommandBuffers(Rendering::Device* device, Device::ThreadResources* resources, vk::CommandBuffer openBuffer)
{
	Data& d = getData();
	for (Any& any : d.m_settings)
	{
		Texture* texture = nullptr;
		if (getResource(texture, any.getPtr<ResourcePtr<Texture>>()) || getResource(texture, any.getPtr<ResourcePtr<TextureAtlas>>()))
		{
			if (submitTextureUpload(device, openBuffer, texture)) return; // upload?
			if (submitLayoutChange(device, openBuffer, texture)) return; // layout change?
		}
	}

	if (submitDrawCall(device, openBuffer)) return;
	if (submitClearCall(device, openBuffer)) return;

	LOG_F(FATAL, "Unknown Unit type\n");
}

bool Unit::submitTextureUpload(Rendering::Device* device, vk::CommandBuffer buffer, Texture* texture)
{
	RenderTarget* target = nullptr;
	Data& d = getData();
	if (d.m_targets.empty())
		return false;

	for (RenderTarget* target : d.m_targets)
	{
		texture->uploadTexture(device, buffer, target);
	}

	return true;
}

bool Unit::submitLayoutChange(Rendering::Device* device, vk::CommandBuffer buffer, Texture* texture)
{
	if (texture->getMode() != Texture::Mode::HOST_VISIBLE)
		return false;

	Data& d = getData();
	for (Any& any : d.m_settings)
	{
		if (any.isType<VkImageLayout>())
		{
			VkImageMemoryBarrier useBuffer[1] = {};
			useBuffer[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			useBuffer[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			useBuffer[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			useBuffer[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			useBuffer[0].newLayout = any.get<VkImageLayout>();
			useBuffer[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBuffer[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBuffer[0].image = texture->getVkImage();
			useBuffer[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			useBuffer[0].subresourceRange.levelCount = 1;
			useBuffer[0].subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, useBuffer);
			return true;
		}
	}

	return false;
}

bool Unit::submitDrawCall(Rendering::Device* device, vk::CommandBuffer buffer)
{
	Data& d = getData();
	Texture* bindTextures[4] = {};
	Buffer *vertices = nullptr, * indices = nullptr;
	auto pipelineLayout = getVulkanObject<vk::PipelineLayout>();
	auto pipeline = getVulkanObject<vk::Pipeline>();
	auto descriptorSet = getVulkanObject<vk::DescriptorSet>();
	std::array<vk::Viewport, 1> viewports = { getVulkanObject<vk::Viewport>() };
	auto scissor = getVulkanObject<vk::Rect2D>();
	if (!pipeline) return false;

	for (Any& any : d.m_settings)
	{
		if ((bindTextures[0] == nullptr || bindTextures[1] == nullptr || bindTextures[2] == nullptr || bindTextures[3] == nullptr) && any.isType<Binding<ResourcePtr<Texture>>>())
		{
			auto& binding = any.get<Binding<ResourcePtr<Texture>>>();
			int stageLocation = binding.m_binding;
			CHECK_F(stageLocation < countof(bindTextures));
			CHECK_F(bindTextures[stageLocation] == nullptr);
			bindTextures[stageLocation] = binding.m_value;
			continue;
		}
		else if ((!vertices || !indices) && any.isType<Named<Buffer*> >())
		{
			auto buffer = any.get<Named< Buffer*> >();
			if (!vertices && buffer.m_value->getType() == Buffer::Type::Vertex) vertices = buffer.m_value;
			else if (!indices && buffer.m_value->getType() == Buffer::Type::Index) indices = buffer.m_value;
			continue;
		}
	}

	if(!vertices)
		return false;

	vk::Buffer vBuffer = vertices->getVkBuffer();
	std::array<vk::DeviceSize, 1> vertexOffset = { 0 };
	buffer.bindVertexBuffers(0, vBuffer, vertexOffset);
	if(indices)
		buffer.bindIndexBuffer(indices->getVkBuffer(), 0, indices->getStride() == 4 ? vk::IndexType::eUint32 : vk::IndexType::eUint16);

	buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, std::array<vk::DescriptorSet, 1>{descriptorSet}, std::array<uint32_t, 0>{0});
	buffer.setViewport(0, viewports);
	buffer.setScissor(0, std::array<vk::Rect2D, 1>{scissor});

	std::size_t pushConstantCount = 0;
	vk::PushConstantRange pushConstantInfo[6] = {};
	for (Any& any : m_data->m_settings)
	{
		if (any.isType<PushConstant>())
		{
			auto& pc = any.get<PushConstant>();
			buffer.pushConstants(pipelineLayout, pc.m_flags, 0, (uint32_t)pc.m_value.size(), &pc.m_value.front());
		}
	}

	vk::ClearValue clearValues;
	clearValues.color = getVulkanObject<vk::ClearColorValue>();
	//clearValues.depthStencil = createVulkanObject<vk::ClearDepthStencilValue>();

	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = device->getRenderPass();
	renderPassInfo.framebuffer = std::get<0>(device->getFrameBuffer());
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValues;
	renderPassInfo.renderArea = scissor;
	buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	for (Any& any : m_data->m_settings)
	{
		if (any.isType<Draw>())
		{
			const Draw& d = any.get<Draw>();
			buffer.draw(d.m_vertexCount, d.m_instanceCount, d.m_firstVertex, d.m_firstInstance);
		}
		else if (any.isType<DrawIndexed>())
		{
			const DrawIndexed& d = any.get<DrawIndexed>();
			buffer.drawIndexed(d.m_indexCount, d.m_instanceCount, d.m_firstIndex, d.m_vertexOffset, d.m_firstInstance);
		}
	}
	buffer.endRenderPass();
	
	return true;
}

bool Unit::submitClearCall(Rendering::Device* device, vk::CommandBuffer buffer)
{
	/*Data& d = getData();
	Texture* bindTextures[4] = {};
	Buffer* vertices = nullptr, * indices = nullptr;
	auto pipelineLayout = getVulkanObject<vk::PipelineLayout>();
	auto pipeline = getVulkanObject<vk::Pipeline>();
	auto descriptorSet = getVulkanObject<vk::DescriptorSet>();*/

	vk::ClearColorValue* clearValue = nullptr;
	for (Any& any : m_data->m_settings)
	{
		if (any.isType<vk::ClearColorValue>())
		{
			clearValue = any.getPtr<vk::ClearColorValue>();
			break;
		}
	}
	
	if (!clearValue)
		return false;

	auto scissor = getVulkanObject<vk::Rect2D>();
	vk::ClearValue clearValues;
	clearValues.color = *clearValue;
	
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = device->getRenderPass();
	renderPassInfo.framebuffer = std::get<0>(device->getFrameBuffer());
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearValues;
	renderPassInfo.renderArea = scissor;
	buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	buffer.endRenderPass();
	return true;
}

template<typename T> Unit& Unit::_in(T v)
{
	CHECK_F(!m_submitted);
	Data& data = getData();
	data.m_empty = false;
	data.m_settings.push_back(v);
	return *this;
}

template<typename T> Unit& Unit::_out(T& v)
{
	CHECK_F(!m_submitted);
	Data& data = getData();
	data.m_empty = false;
	data.m_targets.push_back(&v);
	return *this;
}

Unit& Unit::in(Unit v) { return _in(v); }
Unit& Unit::in(ResourcePtr<Shader> v) { if (v->getType() == Shader::Type::Vertex) return _in(Named<ResourcePtr<Shader>>("Vertex", v)); else return _in(Named<ResourcePtr<Shader>>("Frag", v)); }
Unit& Unit::in(ResourcePtr<Texture> v) { return _in(v); }
Unit& Unit::in(ResourcePtr<TextureAtlas> v) { return _in(v); }
Unit& Unit::in(Buffer* v) {
	if (v->getType() == Buffer::Type::Vertex) return _in(Named<Buffer*>("Vertex", v));
	else if (v->getType() == Buffer::Type::Index)  return _in(Named<Buffer*>("Index", v));
	else return _in(v);
}
Unit& Unit::in(VkImageLayout v) { return _in(v); }
Unit& Unit::in(vk::SamplerMipmapMode v) { return _in(v); }
Unit& Unit::in(vk::CompareOp v) { return _in(v); }
Unit& Unit::in(vk::ClearColorValue v) { return _in(v); }
Unit& Unit::in(vk::PrimitiveTopology v) { return _in(v); }
Unit& Unit::in(Named<bool> v) { return _in(v); }
Unit& Unit::in(Named<float> v) { return _in(v); }
Unit& Unit::in(Named<vk::Filter> v) { return _in(v); }
Unit& Unit::in(Named<vk::SamplerAddressMode> v) { return _in(v); }
Unit& Unit::in(Binding<ResourcePtr<Texture>> v) { return _in(v); }
Unit& Unit::in(PushConstant v) { return _in(v); }
Unit& Unit::in(Draw v) { return _in(v); }
Unit& Unit::in(DrawIndexed v) { return _in(v); }
Unit& Unit::out(Texture& v) { return _out(v); }

// RootUnit
RootUnit::RootUnit():
Unit(*this)
{

}

RootUnit::~RootUnit()
{

}

std::vector<Unit>& RootUnit::getSubmitted()
{
	return m_submitted;
}

void Unit::test()
{

}
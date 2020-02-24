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

// Visitor: T* (Data&, PropagateFunction);
template<typename T, typename Visitor> 
T* Unit::constructWithVisitor(Visitor v)
{
	std::function<T*(Unit&)> propagate = [&v, &propagate](Unit& unit) -> T*
	{
		Data& data = unit.getData();
		for(auto it = data.m_supers.rbegin(); it != data.m_supers.rend(); ++it)
		{
			if (T* t = v(it->getData(), propagate))
				return t;
		}

		return nullptr;
	};

	return v(getData(), propagate);
}

#define IF_VALUE(name) if(strcmp(value->m_name, #name) == 0) { info.name = value->m_value; }
#define ELSE_IF_VALUE(name) else IF_VALUE(name)

template<>
vk::Sampler* Unit::getVulkanObject<vk::Sampler>()
{
	Data& data = getData();
	if (!data.m_sampler)
	{
		Masked<vk::SamplerCreateInfo> info;
		data.m_sampler = constructWithVisitor<vk::Sampler>([&info](Data& data, std::function<vk::Sampler* (Unit&)>& propagate) -> vk::Sampler* {
			for (auto& it : data.m_settings)
			{
				if (auto* value = it.getPtr< Named<vk::Filter> >())
				{
					IF_VALUE(magFilter)
					else IF_VALUE(minFilter)
				}
				else if (auto* value = it.getPtr< Named<vk::SamplerAddressMode> >())
				{
					IF_VALUE(addressModeU)
					ELSE_IF_VALUE(addressModeV)
					ELSE_IF_VALUE(addressModeW)
				}
				else if (auto* value = it.getPtr< Named<float> >())
				{
					IF_VALUE(mipLodBias)
					ELSE_IF_VALUE(maxAnisotropy)
					ELSE_IF_VALUE(minLod)
					ELSE_IF_VALUE(maxLod)
				}
				else if (auto* value = it.getPtr< Named<bool> >())
				{
					IF_VALUE(anisotropyEnable)
					ELSE_IF_VALUE(compareEnable)
					ELSE_IF_VALUE(unnormalizedCoordinates)
				}
				else if (auto* value = it.getPtr< Named<vk::CompareOp> >())
				{
					IF_VALUE(compareOp)
				}
			}

			for (auto& supers : data.m_supers)
			{
				if (vk::Sampler* s = propagate(supers))
					return s;
			}

			ResourcePtr<Device> device;
			return device->createObject(info);
		});
	}
	
	return static_cast<vk::Sampler*>(data.m_sampler);
}

/*
typedef struct VkDescriptorSetLayoutBinding {
uint32_t              binding;
VkDescriptorType      descriptorType;
uint32_t              descriptorCount;
VkShaderStageFlags    stageFlags;
const VkSampler*      pImmutableSamplers;
} VkDescriptorSetLayoutBinding;
*/

template<typename T>
static bool ProcessBinding(Any& any, vk::DescriptorType type, std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
	if (!any.isType<Unit::Binding<T>>())
		return false;

	auto binding = any.get<Unit::Binding<T>>();
	auto it = std::find_if(bindings.begin(), bindings.end(), [&binding](const vk::DescriptorSetLayoutBinding& vkbinding) { return binding.m_stage == vkbinding.binding; });
	if (it == bindings.end())
	{
		bindings.emplace_back(binding.m_stage, type, 1, vk::ShaderStageFlagBits::eAllGraphics, nullptr);
	}

	return true;
}

template<>
static bool ProcessBinding<Unit>(Any& any, vk::DescriptorType type, std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
	if (!any.isType<Unit::Binding<Unit>>())
		return false;

	auto binding = any.get<Unit::Binding<Unit>>();
	return true;
}

template<>
vk::DescriptorSetLayout* Unit::getVulkanObject<vk::DescriptorSetLayout>()
{
	Data& data = getData();
	if (!data.m_descriptorSetLayout)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		data.m_descriptorSetLayout = constructWithVisitor<vk::DescriptorSet>([&data, &bindings](Data& data, std::function<vk::DescriptorSet*(Unit&)> propagate)
		{
			for (Any& setting : data.m_settings)
			{
				if (ProcessBinding<ResourcePtr<Texture>>(setting, vk::DescriptorType::eSampledImage, bindings)) continue;
				if (ProcessBinding<ResourcePtr<Unit>>(setting, vk::DescriptorType::eSampler, bindings)) continue;
			}

			for (auto& unit : data.m_supers)
				if (vk::DescriptorSet* set = propagate(unit))
					return set;

			return (vk::DescriptorSet*)nullptr;
		});
	}
	return static_cast<vk::DescriptorSetLayout*>(data.m_descriptorSetLayout);
}

#undef IF_VALUE
#undef ELSE_IF_VALUE

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

	LOG_F(FATAL, "Unknown Unit type");
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
	Shader *vShader = nullptr, *fShader = nullptr;
	for (Any& any : d.m_settings)
	{
		if ((!vShader|| !fShader) && any.isType< ResourcePtr<Shader> >())
		{
			Shader* shader = any.get<ResourcePtr<Shader>>();
			if (!vShader && shader->getType() == Shader::Type::Vertex)	vShader = shader;
			else														fShader = shader;

			continue;
		}
		else if ((bindTextures[0] == nullptr || bindTextures[1] == nullptr || bindTextures[2] == nullptr || bindTextures[3] == nullptr) && any.isType<Binding<ResourcePtr<Texture>>>())
		{
			auto& binding = any.get<Binding<ResourcePtr<Texture>>>();
			int stage = binding.m_stage;
			CHECK_F(stage < countof(bindTextures));
			CHECK_F(bindTextures[stage] == nullptr);
			bindTextures[stage] = binding.m_value;
			continue;
		}
		else if ((!vertices || !indices) && any.isType<Buffer*>())
		{
			Buffer* buffer = any.get<Buffer*>();
			if (!vertices && buffer->getType() == Buffer::Type::Vertex) vertices = buffer;
			else if (!indices && buffer->getType() == Buffer::Type::Index) indices = buffer;
			continue;
		}
	}

	if(!vertices || !indices || !vShader || !fShader || !bindTextures[0])
		return false;

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
Unit& Unit::in(ResourcePtr<Shader> v) { return _in(v); }
Unit& Unit::in(ResourcePtr<Texture> v) { return _in(v); }
Unit& Unit::in(ResourcePtr<TextureAtlas> v) { return _in(v); }
Unit& Unit::in(Buffer* v) { return _in(v); }
Unit& Unit::in(VkImageLayout v) { return _in(v); }
Unit& Unit::in(vk::SamplerMipmapMode v) { return _in(v); }
Unit& Unit::in(vk::CompareOp v) { return _in(v); }
Unit& Unit::in(Named<bool> v) { return _in(v); }
Unit& Unit::in(Named<float> v) { return _in(v); }
Unit& Unit::in(Named<vk::Filter> v) { return _in(v); }
Unit& Unit::in(Named<vk::SamplerAddressMode> v) { return _in(v); }
Unit& Unit::in(Binding<ResourcePtr<Texture>> v) { return _in(v); }
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
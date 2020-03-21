#include "stdafx.h"
#include "Unit.h"
#include "Shader.h"

namespace Rendering {

template<typename T, typename Visitor>
T Unit::constructWithVisitor(Visitor v)
{
	std::function<T(Unit&)> propagate = [&v, &propagate](Unit& unit) -> T
	{
		Data& data = unit.getData();
		for (auto it = data.m_supers.rbegin(); it != data.m_supers.rend(); ++it)
		{
			if (T t = v(it->getData(), propagate))
				return t;
		}

		return nullptr;
	};

	return v(getData(), propagate);
}

#define IF_VALUE(name) if(strcmp(value->m_name, #name) == 0) { info.name = value->m_value; }
#define ELSE_IF_VALUE(name) else IF_VALUE(name)

template<>
vk::Sampler Unit::getVulkanObject<vk::Sampler>()
{
	Data& data = getData();
	if (!data.m_sampler)
	{
		Masked<vk::SamplerCreateInfo> info;
		data.m_sampler = constructWithVisitor<vk::Sampler>([&info](Data& data, std::function<vk::Sampler(Unit&)>& propagate) -> vk::Sampler {
			for (auto& it : data.m_settings)
			{
				if (auto* value = it.getPtr< Named<vk::Filter> >())
				{
					IF_VALUE(magFilter)
					ELSE_IF_VALUE(minFilter)
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
				if (vk::Sampler s = propagate(supers))
					return s;
			}

			ResourcePtr<Device> device;
			return device->createObject(info);
		});
	}

	return *static_cast<vk::Sampler*>(data.m_sampler);
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
vk::DescriptorSetLayout Unit::getVulkanObject<vk::DescriptorSetLayout>()
{
	Data& data = getData();
	if (!data.m_descriptorSetLayout)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		data.m_descriptorSetLayout = constructWithVisitor<vk::DescriptorSet>([&data, &bindings](Data& data, std::function<vk::DescriptorSet(Unit&)> propagate)
		{
			for (Any& setting : data.m_settings)
			{
				if (ProcessBinding<ResourcePtr<Texture>>(setting, vk::DescriptorType::eSampledImage, bindings)) continue;
				if (ProcessBinding<ResourcePtr<Unit>>(setting, vk::DescriptorType::eSampler, bindings)) continue;
			}

			for (auto& unit : data.m_supers)
				if (vk::DescriptorSet set = propagate(unit))
					return set;

			return static_cast<vk::DescriptorSet>(nullptr);
		});
	}
	return *static_cast<vk::DescriptorSetLayout*>(data.m_descriptorSetLayout);
}

template<>
vk::Pipeline Unit::getVulkanObject<vk::Pipeline>()
{
	vk::GraphicsPipelineCreateInfo info;
	Data& data = getData();
	if (!data.m_pipeline)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		data.m_pipeline = constructWithVisitor<vk::Pipeline>([&data, &bindings](Data& data, std::function<vk::Pipeline(Unit&)> propagate)
		{
			Shader* vShader = nullptr, * fShader = nullptr;
			for (Any& setting : data.m_settings)
			{
				if (setting.isType<Shader>())
				{
					Shader* shader = &setting.get<Shader>();
					if (shader->getType() == Shader::Type::Vertex)		vShader = shader;
					else if (shader->getType() == Shader::Type::Pixel)	fShader = shader;
					else												LOG_F(ERROR, "Unknown shader type\n");
				}
			}

			for (auto& unit : data.m_supers)
				if (vk::Pipeline pipeline = propagate(unit))
					return pipeline;

			return static_cast<vk::Pipeline>(nullptr);
		});
	}
	return *static_cast<vk::Pipeline*>(data.m_descriptorSetLayout);
}

#undef IF_VALUE
#undef ELSE_IF_VALUE
}
#include "stdafx.h"
#include "Unit.h"
#include "Buffer.h"
#include "Shader.h"
#include "../Misc/Misc.h"
#include "Texture.h"

// there's a lot of looping here, hopefully it's ok

namespace Rendering {

template<>
vk::Sampler Unit::createVulkanObject<vk::Sampler>()
{
	Data& data = getData();
	if (!data.m_sampler)
	{
        ResourcePtr<Device> device;
		vk::SamplerCreateInfo info;
        opt(info.flags);
        opt(info.magFilter, "magFilter");
        opt(info.minFilter, "minFilter");
        opt(info.mipmapMode);
        opt(info.addressModeU, "addressModeU");
        opt(info.addressModeV, "addressModeV");
        opt(info.addressModeW, "addressModeW");
        opt(info.mipLodBias, "mipLodBias");
        opt(info.anisotropyEnable, "anisotropyEnable");
        opt(info.maxAnisotropy, "maxAnisotropy");
        opt(info.compareEnable, "compareEnable");
        opt(info.compareOp);
        opt(info.minLod, "minLod");
        opt(info.maxLod, "maxLod");
        opt(info.borderColor);
        opt(info.unnormalizedCoordinates, "unnormalizedCoordinates");
        data.m_sampler = device->createObject(info);
	}

	return data.m_sampler;
}

template<typename T>
inline void Unit::getBindings(Data* data, std::vector< vk::DescriptorSetLayoutBinding>* bindings, vk::DescriptorType type)
{
    for (Any& any : data->m_settings)
    {
        auto* b = any.getPtr<Binding<T>>();
        if (b)
        {
            vk::DescriptorSetLayoutBinding binding;
            binding.descriptorType = type;
            binding.stageFlags = b->m_flags;
            binding.binding = b->m_binding;
            binding.descriptorCount = 1;
            if(std::find(bindings->begin(), bindings->end(), binding) == bindings->end())
                bindings->push_back(binding);
        }
    }
    for (auto& s : data->m_supers)
        getBindings<T>(data, bindings, type);
}

template<>
vk::DescriptorSet Unit::createVulkanObject<vk::DescriptorSet>()
{
    Data& data = getData();
    if (!data.m_descriptorSet)
    {
        ResourcePtr<Device> device;
        auto layout = getVulkanObject<vk::DescriptorSetLayout>();
        vk::DescriptorSetAllocateInfo info;
        info.descriptorPool = device->getDescriptorPool();
        info.descriptorSetCount = 1;
        info.pSetLayouts = &layout;
        data.m_descriptorSet = device->allocateObject(info);

        std::vector<vk::WriteDescriptorSet> writes;
        std::list<vk::DescriptorImageInfo> imageInfos;
        for (auto& any : data.m_settings)
        {
            if (auto* bind = any.getPtr<Binding<ResourcePtr<Texture>>>())
            {
                Texture* texture = bind->m_value;
                imageInfos.emplace_front(texture->getSampler(0).getVulkanObject<vk::Sampler>(), texture->getImageView() , texture->getImageLayout());
                writes.emplace_back(data.m_descriptorSet, bind->m_binding, 0, (uint32_t)imageInfos.size(), vk::DescriptorType::eCombinedImageSampler, &imageInfos.front());
            }
            else if (auto* bind = any.getPtr<Binding<Buffer*>>())
            {
                /*
                    VULKAN_HPP_CONSTEXPR WriteDescriptorSet( VULKAN_HPP_NAMESPACE::DescriptorSet dstSet_ = {},
                    uint32_t dstBinding_ = {},
                    uint32_t dstArrayElement_ = {},
                    uint32_t descriptorCount_ = {},
                    VULKAN_HPP_NAMESPACE::DescriptorType descriptorType_ = VULKAN_HPP_NAMESPACE::DescriptorType::eSampler,
                    const VULKAN_HPP_NAMESPACE::DescriptorImageInfo* pImageInfo_ = {},
                    const VULKAN_HPP_NAMESPACE::DescriptorBufferInfo* pBufferInfo_ = {},
                    const VULKAN_HPP_NAMESPACE::BufferView* pTexelBufferView_ = {} ) VULKAN_HPP_NOEXCEPT

                    ( VULKAN_HPP_NAMESPACE::Buffer buffer_ = {},
                                               VULKAN_HPP_NAMESPACE::DeviceSize offset_ = {},
                                               VULKAN_HPP_NAMESPACE::DeviceSize range_ = {} )

                    */
                LOG_IF_F(ERROR, bind->m_value->getType() != Buffer::Type::Uniform, "Not a uniform buffer");
                vk::DescriptorBufferInfo bufferInfo(bind->m_value->getVkBuffer(), 0, VK_WHOLE_SIZE);
                writes.emplace_back(data.m_descriptorSet, bind->m_binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo);
            }
        }

        if(!writes.empty())
            device->updateObject(writes);
    }
    return data.m_descriptorSet;
}

template<>
vk::DescriptorSetLayout Unit::createVulkanObject<vk::DescriptorSetLayout>()
{
	Data& data = getData();
    if (!data.m_descriptorSetLayout)
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        getBindings<ResourcePtr<Texture>>(&data, &bindings, vk::DescriptorType::eCombinedImageSampler);
        getBindings<Buffer*>(&data, &bindings, vk::DescriptorType::eUniformBuffer);

        ResourcePtr<Device> device;
        vk::DescriptorSetLayoutCreateInfo info;
        info.bindingCount = (uint32_t)bindings.size();
        info.pBindings = info.bindingCount ? &bindings[0] : nullptr;
        data.m_descriptorSetLayout = device->createObject(info);
    }
	return data.m_descriptorSetLayout;
}

template<>
vk::PipelineLayout Unit::createVulkanObject<vk::PipelineLayout>()
{
    Data& data = getData();
    if (!data.m_pipelineLayout)
    {
        ResourcePtr<Device> device;
        vk::DescriptorSetLayout setLayout[1] = { getVulkanObject<vk::DescriptorSetLayout>() };
        vk::PipelineLayoutCreateInfo info = {};
        info.setLayoutCount = 1;
        info.pSetLayouts = setLayout;

        std::size_t pushConstantCount = 0;
        vk::PushConstantRange pushConstantInfo[6] = {};
        for (Any& any : m_data->m_settings)
        {
            if (any.isType<PushConstant>())
            {
                auto& pc = any.get<PushConstant>();
                auto& pcInfo = pushConstantInfo[pushConstantCount++];
                pcInfo.stageFlags = pc.m_flags;
                pcInfo.offset = 0;
                pcInfo.size = (uint32_t)pc.m_value.size();
            }
        }
        info.pushConstantRangeCount = (uint32_t)pushConstantCount;
        info.pPushConstantRanges = pushConstantInfo;

        data.m_pipelineLayout = device->createObject(info);
    }
    return data.m_pipelineLayout;
}

template<>
vk::Pipeline Unit::createVulkanObject<vk::Pipeline>()
{
    Data& data = getData();
    if (!data.m_pipeline)
    {
        ResourcePtr<Device> device;
        vk::GraphicsPipelineCreateInfo info;
        opt(info.flags);

        ResourcePtr<Shader> vertexShader(EmptyPtr), fragShader(EmptyPtr);
        req(vertexShader, "Vertex");
        req(fragShader, "Frag");

        Buffer* vertexBuffer = nullptr, *indexBuffer = nullptr;
        req(vertexBuffer, "Vertex");
        opt(indexBuffer, "Index");
        if (vertexBuffer->getFormat().empty() || (indexBuffer && indexBuffer->getFormat().empty()))
            throw std::runtime_error{ stringf("%s buffer did not have format set", vertexBuffer->getFormat().empty() ? "Vertex" : "Index")};

        vk::PipelineShaderStageCreateInfo stages[2];
        stages[0].stage = vk::ShaderStageFlagBits::eVertex;
        stages[0].module = vertexShader->getModule();
        stages[0].pName = "main";
        stages[1].stage = vk::ShaderStageFlagBits::eFragment;
        stages[1].module = fragShader->getModule();
        stages[1].pName = "main";
        info.stageCount = 2;
        info.pStages = stages;

        vk::PipelineRasterizationStateCreateInfo rasterization;
        opt(rasterization.polygonMode);
        opt<vk::CullModeFlags>(rasterization.cullMode, nullptr, vk::CullModeFlagBits::eBack);
        opt(rasterization.frontFace);
        opt(rasterization.lineWidth, "lineWidth", 1.0f);

        info.pRasterizationState = &rasterization;

        vk::PipelineViewportStateCreateInfo viewport;
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        info.pViewportState = &viewport;

        vk::PipelineMultisampleStateCreateInfo multisample;
        info.pMultisampleState = &multisample;

        info.layout = getVulkanObject<vk::PipelineLayout>();

        info.renderPass = device->getRenderPass();

        vk::DynamicState dynamicStates[2] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;
        info.pDynamicState = &dynamicState;

        Buffer *vBuffer = nullptr, *iBuffer = nullptr;
        req(vBuffer, "Vertex");
        opt(iBuffer, "Index");

        vk::VertexInputBindingDescription vertexBinding[1] = {};
        vertexBinding[0].stride = (uint32_t)vBuffer->getStride();
        vertexBinding[0].inputRate = vk::VertexInputRate::eVertex;

        const std::vector<Buffer::Format>& vertexFormat = vBuffer->getFormat();
        vk::VertexInputAttributeDescription* vertexAttributes = (vk::VertexInputAttributeDescription*)alloca(sizeof(vk::VertexInputAttributeDescription) * vertexFormat.size());
        uint32_t offset = 0;
        for (std::size_t i = 0; i < vertexFormat.size(); i++)
        {
            vertexAttributes[i].location = (uint32_t)i;
            vertexAttributes[i].binding = vertexBinding[0].binding;
            vertexAttributes[i].format = vertexFormat[i].m_format;
            vertexAttributes[i].offset = offset;
            offset += (uint32_t)vertexFormat[i].m_size;
        }

        vk::PipelineVertexInputStateCreateInfo vertexInfo = {};
        vertexInfo.vertexBindingDescriptionCount = 1;
        vertexInfo.pVertexBindingDescriptions = vertexBinding;
        vertexInfo.vertexAttributeDescriptionCount = (uint32_t)vertexFormat.size();
        vertexInfo.pVertexAttributeDescriptions = vertexAttributes;
        info.pVertexInputState = &vertexInfo;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
        opt(inputAssemblyState.topology, nullptr, vk::PrimitiveTopology::eTriangleStrip);
        info.pInputAssemblyState = &inputAssemblyState;

        vk::PipelineColorBlendAttachmentState colorBlend[1] = {};
        colorBlend[0].blendEnable = VK_TRUE;
        colorBlend[0].srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlend[0].dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlend[0].colorBlendOp = vk::BlendOp::eAdd;
        colorBlend[0].srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlend[0].dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlend[0].alphaBlendOp = vk::BlendOp::eAdd;
        colorBlend[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineDepthStencilStateCreateInfo depthInfo;
        DepthTest depthTest(vk::CompareOp::eLess, false, false);
        opt<DepthTest>(depthTest);
        if (depthTest.m_depthTestEnable || depthTest.m_depthWriteEnable)
        {
            depthInfo.depthTestEnable = depthTest.m_depthTestEnable ? VK_TRUE : VK_FALSE;
            depthInfo.depthWriteEnable = depthTest.m_depthWriteEnable ? VK_TRUE : VK_FALSE;
            depthInfo.depthCompareOp = depthTest.m_depthCompareOp;
        }
        else
        {
            depthInfo.depthTestEnable = VK_FALSE;
        }
        depthInfo.depthBoundsTestEnable = VK_FALSE;
        depthInfo.maxDepthBounds = 1000.0f;
        depthInfo.stencilTestEnable = VK_FALSE;

        vk::PipelineColorBlendStateCreateInfo blendInfo;
        blendInfo.attachmentCount = 1;
        blendInfo.pAttachments = colorBlend;

        info.pColorBlendState = &blendInfo;
        info.pDepthStencilState = &depthInfo;

        /*
        const VULKAN_HPP_NAMESPACE::PipelineTessellationStateCreateInfo* pTessellationState = {};
        const VULKAN_HPP_NAMESPACE::PipelineViewportStateCreateInfo* pViewportState = {};
        
        const VULKAN_HPP_NAMESPACE::PipelineMultisampleStateCreateInfo* pMultisampleState = {};
        const VULKAN_HPP_NAMESPACE::PipelineDepthStencilStateCreateInfo* pDepthStencilState = {};
        const VULKAN_HPP_NAMESPACE::PipelineDynamicStateCreateInfo* pDynamicState = {};*/
        //
        /*
        uint32_t subpass = {};
        VULKAN_HPP_NAMESPACE::Pipeline basePipelineHandle = {};
        int32_t basePipelineIndex = {};*/

        data.m_pipeline = device->createObject(info);
    }
	return data.m_pipeline;
}

template<>
vk::Viewport Unit::createVulkanObject<vk::Viewport>()
{
    ResourcePtr<Device> device;
    glm::vec2 d = std::get<1>(device->getFrameBuffer());
    return {0, 0, d.x, d.y, 0.0f, 1.0f};
}

template<>
vk::Rect2D Unit::createVulkanObject<vk::Rect2D>()
{
    ResourcePtr<Device> device;
    glm::vec2 d = std::get<1>(device->getFrameBuffer());
    return { {0, 0}, {(uint32_t)d.x, (uint32_t)d.y} };
}

template<>
vk::ClearColorValue Unit::createVulkanObject<vk::ClearColorValue>()
{
    vk::ClearColorValue c;
    opt<vk::ClearColorValue>(c, nullptr, std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f});
    return c;
}

template<>
vk::ClearDepthStencilValue Unit::createVulkanObject<vk::ClearDepthStencilValue>()
{
    vk::ClearDepthStencilValue stencil;
    opt<vk::ClearDepthStencilValue>(stencil, nullptr, {1.0f, 0});
    return stencil;
}

}
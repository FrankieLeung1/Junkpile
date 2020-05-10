#pragma once
#undef IN
#undef OUT

#include <vector>
#include "../Misc/Any.h"
#include "../Misc/ClassMask.h"
#include "RenderingDevice.h"
namespace Rendering
{
	class Shader;
	class RootUnit;
	class Buffer;
	class Texture;
	class TextureAtlas;
	class UnitInstance;
	class RenderTarget;
	typedef std::shared_ptr<UnitInstance> SubmitCache;

	class Unit
	{
	public:
		template<typename T>
		struct Named
		{
			Named(const char* name, T value) : m_name(name), m_value(value) {}
			const char* m_name;
			T m_value;
		};

		template<typename T>
		struct Binding
		{
			Binding(vk::ShaderStageFlags flags, int binding, T value): m_flags(flags), m_binding(binding), m_value(value) {}
			vk::ShaderStageFlags m_flags;
			int m_binding;
			T m_value;
		};

		struct PushConstant
		{
			PushConstant(vk::ShaderStageFlags flags, std::vector<char>&& value): m_flags(flags), m_value(std::move(value)) {}
			vk::ShaderStageFlags m_flags;
			std::vector<char> m_value;
		};

		struct Draw
		{
			Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance): m_vertexCount(vertexCount), m_instanceCount(instanceCount), m_firstVertex(firstVertex), m_firstInstance(firstInstance) {}
			uint32_t m_vertexCount, m_instanceCount, m_firstVertex, m_firstInstance;
		};

		struct DrawIndexed
		{
			DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) : m_indexCount(indexCount), m_instanceCount(instanceCount), m_firstIndex(firstIndex), m_vertexOffset(vertexOffset), m_firstInstance(firstInstance){}
			uint32_t m_indexCount, m_instanceCount, m_firstIndex, m_vertexOffset, m_firstInstance;
		};

		
		
	public:
		Unit();
		Unit(RootUnit&);
		Unit(const Unit&);
		~Unit();
		
		void submit();
		bool isEmpty() const;

		Unit& operator=(const Unit&);
		bool operator==(const Unit&) const;

		Unit& in(Unit);
		Unit& in(ResourcePtr<Shader>);
		Unit& in(ResourcePtr<Texture>);
		Unit& in(ResourcePtr<TextureAtlas>);
		Unit& in(Buffer*);
		Unit& in(vk::PrimitiveTopology);
		Unit& in(vk::CullModeFlags v);
		Unit& in(VkImageLayout);
		Unit& in(vk::SamplerMipmapMode);
		Unit& in(vk::CompareOp);
		Unit& in(vk::ClearColorValue);
		Unit& in(Named<bool>);
		Unit& in(Named<float>);
		Unit& in(Named<vk::Filter>);
		Unit& in(Named<vk::SamplerAddressMode>);
		Unit& in(Binding<ResourcePtr<Texture>>);
		Unit& in(PushConstant);
		Unit& in(Draw);
		Unit& in(DrawIndexed);
		Unit& out(Texture&);

		static void test();

	protected:
		struct Data;
		template<typename T> Unit& _in(T);
		template<typename T> Unit& _out(T&);
		template<typename T> T getVulkanObject();
		template<typename T> T createVulkanObject();
		template<typename T> void reqOrOpt(T&, Any index, bool required);
		template<typename T> void req(T&, const char* name = nullptr);
		template<typename T> void opt(T&, const char* name = nullptr);
		template<typename T> void req(T&, int binding);
		template<typename T> void opt(T&, int binding);
		template<typename T> void opt(T& v, const char* name, const T& defaultValue);
		template<typename T> void opt(T&, int binding, const T& defaultValue);
		template<typename T> void getBindings(Data* data, std::vector< vk::DescriptorSetLayoutBinding>* bindings, vk::DescriptorType type);

		Data& getData();

		void submitCommandBuffers(Rendering::Device*, Device::ThreadResources*, vk::CommandBuffer openBuffer);
		bool submitTextureUpload(Rendering::Device*, vk::CommandBuffer, Texture*);
		bool submitLayoutChange(Rendering::Device* device, vk::CommandBuffer buffer, Texture* texture);
		bool submitDrawCall(Rendering::Device* device, vk::CommandBuffer buffer);
		bool submitClearCall(Rendering::Device* device, vk::CommandBuffer buffer);

	protected:
		struct Data
		{
			RootUnit* m_root{ nullptr };
			std::vector<Unit> m_supers;
			std::vector<RenderTarget*> m_targets;
			std::vector<Any> m_settings;

			vk::Sampler m_sampler;
			vk::DescriptorSetLayout m_descriptorSetLayout;
			vk::DescriptorSet m_descriptorSet;
			vk::Pipeline m_pipeline;
			vk::PipelineLayout m_pipelineLayout;
			bool m_empty{ true };
		};
		std::shared_ptr<Data> m_data;
		bool m_submitted;

		friend class Rendering::Device;
	};

	class RootUnit : public Unit
	{
	public:
		std::vector<Unit>& getSubmitted();

	protected:
		RootUnit();
		~RootUnit();

	protected:
		std::vector<Unit> m_submitted;

		friend class Device;
		friend class Unit;
	};

	// ---------------------------------- IMPLEMENTATION ----------------------------------
	template<typename T>
	T Unit::getVulkanObject()
	{
		try
		{
			return createVulkanObject<T>();
		}
		catch (std::exception e)
		{
			//LOG_F(ERROR, "Unable to get vulkan object %s\n", e.what());
		}

		return {};
	}
}
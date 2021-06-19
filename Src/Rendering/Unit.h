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
	class Surface;
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

		struct DepthTest
		{
			DepthTest(vk::CompareOp op = vk::CompareOp::eLess, bool testEnable = true, bool writeEnable = true) : m_depthCompareOp(op), m_depthTestEnable(testEnable), m_depthWriteEnable(writeEnable) {}
			vk::CompareOp m_depthCompareOp;
			bool m_depthTestEnable = {};
			bool m_depthWriteEnable = {};
			
			/*bool depthBoundsTestEnable = {};
			float minDepthBounds = {};
			float maxDepthBounds = {};*/
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
		Unit& in(vk::CullModeFlags);
		Unit& in(vk::ImageLayout);
		Unit& in(vk::ImageAspectFlags);
		Unit& in(vk::SamplerMipmapMode);
		Unit& in(vk::CompareOp);
		Unit& in(vk::ClearColorValue);
		Unit& in(vk::Image);
		Unit& in(const DepthTest&);
		Unit& in(Named<bool>);
		Unit& in(Named<float>);
		Unit& in(Named<vk::Filter>);
		Unit& in(Named<vk::SamplerAddressMode>);
		Unit& in(Binding<ResourcePtr<Texture>>);
		Unit& in(Binding<Buffer*>);
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

		enum class CommandType { Unknown, LayoutChange, Upload, Draw, Clear};
		CommandType submitCommandBuffers(Rendering::Device*, Device::ThreadResources*, vk::CommandBuffer openBuffer, bool clear);
		bool submitTextureUpload(Rendering::Device*, vk::CommandBuffer, Texture*);
		bool submitLayoutChange(Rendering::Device* device, vk::CommandBuffer buffer, Texture* texture);
		bool submitLayoutChange(Rendering::Device* device, vk::CommandBuffer buffer, vk::Image image);
		bool submitDrawCall(Rendering::Device* device, vk::CommandBuffer buffer, bool clear);
		bool submitClearCall(Rendering::Device* device, vk::CommandBuffer buffer);

	protected:
		struct Data
		{
			RootUnit* m_root{ nullptr };
			std::vector<Unit> m_supers;
			std::vector<Surface*> m_targets;
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
		std::string m_stack;
		std::jmp_buf m_jmpPoint;
		ResourcePtr<Rendering::Device> m_device;

		friend class Rendering::Device;
	};

	// Frankie: This is basically an array of Units. Doesn't seem like this is needed anymore
	class RootUnit
	{
	public:
		std::vector<Unit>& getSubmitted();
		void clearSubmitted();

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
		if (!std::setjmp(m_jmpPoint))
		{
			return createVulkanObject<T>();
		}
		else
		{
			return {};
		}
	}

	template<typename T> struct AnyType { typedef T Type; };
	template<> struct AnyType<vk::Bool32> { typedef bool Type; };

	template<typename T> void Unit::reqOrOpt(T& v, Any index, bool required)
	{
		Data& data = getData();
		int bindingStage = index.isType<int>() ? index.get<int>() : -1;
		const char* name = index.isType<const char*>() ? index.get<const char*>() : nullptr;
		for (auto& any : data.m_settings)
		{
			if (bindingStage >= 0)
			{
				if (any.isType< AnyType<T>::Type >())
				{
					auto& binding = any.get< Binding<AnyType<T>::Type> >();
					if (bindingStage == binding.m_binding)
					{
						v = (T)binding.m_value;
						return;
					}
				}
			}
			else if (name)
			{
				if (any.isType< Named<AnyType<T>::Type > >())
				{
					auto& named = any.get< Named<AnyType<T>::Type> >();
					if (strcmp(name, named.m_name) == 0)
					{
						v = (T)named.m_value;
						return;
					}
				}
			}
			else
			{
				if (any.isType< AnyType<T>::Type >())
				{
					v = (T)any.get< AnyType<T>::Type >();
					return;
				}
			}
		}

		for (auto& super : data.m_supers)
		{
			try { super.req(v, name); }
			catch (...) {}
		}

		if (required)
			std::longjmp(m_jmpPoint, -1);
			//throw std::runtime_error{ stringf("Failed to find Vulkan variable %s", name ? name : typeid(T).name()) };
	}

	template<typename T> void Unit::req(T& v, const char* name) { reqOrOpt(v, name, true); }
	template<typename T> void Unit::opt(T& v, const char* name) { reqOrOpt(v, name, false); }
	template<typename T> void Unit::req(T& v, int binding) { reqOrOpt(v, binding, true); }
	template<typename T> void Unit::opt(T& v, int binding) { reqOrOpt(v, binding, false); }
	template<typename T> void Unit::opt(T& v, const char* name, const T& defaultValue) { v = defaultValue; reqOrOpt(v, name, false); }
	template<typename T> void Unit::opt(T& v, int binding, const T& defaultValue) { v = defaultValue; reqOrOpt(v, binding, false); }
}
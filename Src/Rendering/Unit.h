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
			Binding(int stage, T value): m_stage(stage), m_value(value) {}
			int m_stage;
			T m_value;
		};
		
	public:
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
		Unit& in(VkImageLayout);
		Unit& in(vk::SamplerMipmapMode);
		Unit& in(vk::CompareOp);
		Unit& in(Named<bool>);
		Unit& in(Named<float>);
		Unit& in(Named<vk::Filter>);
		Unit& in(Named<vk::SamplerAddressMode>);
		Unit& in(Binding<ResourcePtr<Texture>>);
		Unit& out(Texture&);

		static void test();

	protected:
		template<typename T> Unit& _in(T);
		template<typename T> Unit& _out(T&);
		template<typename T, typename Visitor> T* constructWithVisitor(Visitor);
		template<typename T> T* getVulkanObject();

		struct Data;
		Data& getData();

		void submitCommandBuffers(Rendering::Device*, Device::ThreadResources*, vk::CommandBuffer openBuffer);
		bool submitTextureUpload(Rendering::Device*, vk::CommandBuffer, Texture*);
		bool submitLayoutChange(Rendering::Device* device, vk::CommandBuffer buffer, Texture* texture);
		bool submitDrawCall(Rendering::Device* device, vk::CommandBuffer buffer);

	protected:
		struct Data
		{
			RootUnit* m_root{ nullptr };
			std::vector<Unit> m_supers;
			std::vector<RenderTarget*> m_targets;
			std::vector<Any> m_settings;

			void* m_sampler;
			void* m_descriptorSetLayout;
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
}
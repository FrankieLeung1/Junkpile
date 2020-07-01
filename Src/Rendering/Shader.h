#pragma once

#include <tuple>
#include <string>

namespace Rendering
{
	struct Binding
	{
		enum class Type { Texture = 0, Buffer, Constant, PushConstant, Max };
		Type m_type;

		// name or index
		int m_index;
		std::string m_name;
		vk::Format m_format;
		std::size_t m_offset;

		Binding();
		Binding(Type, int, vk::Format, std::size_t offset);
		Binding(Type, const char*, vk::Format, std::size_t offset);
		~Binding();
		bool operator==(const Binding&) const;
	};

	class Shader : public Resource
	{
	public:
		enum class Type { Vertex, Pixel };

	public:
		Shader();
		Shader(Type, const char*);
		~Shader();

		std::string getCachePath() const;
		vk::ShaderModule getModule() const;

		void setCode(Type, const char*);
		bool compile(std::string*);

		Type getType() const;
		static void test();

		class ShaderLoader : public Loader
		{
		public:
			ShaderLoader(Type, const char*);
			~ShaderLoader();
			Shader* load(std::tuple<int, std::string>* error);
			std::string getDebugName() const;
			StringView getTypeName() const;

		protected:
			ResourcePtr<Device> m_device;
			Type m_type;
			std::string m_code;
		};
		static ShaderLoader* createLoader(Type, const char*);
		static std::tuple<bool, std::size_t> getSharedHash(Type, const char*);

	protected:
		void freeModule();

	protected:
		Type m_type;
		std::string m_code;
		std::size_t m_hash;
		
		vk::ShaderModule m_module;
	};
}
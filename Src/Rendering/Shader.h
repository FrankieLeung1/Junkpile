#pragma once

#include <tuple>
#include <string>

namespace Rendering
{
	class Shader : public Resource
	{
	public:
		enum class Type { Vertex, Pixel };

	public:
		Shader();
		Shader(Type, const char*);
		~Shader();

		std::string getCachePath() const;

		void setCode(Type, const char*);
		bool compile(std::string*);

		static void test();

		class ShaderLoader : public Loader
		{
		public:
			ShaderLoader(Type, const char*);
			~ShaderLoader();
			Shader* load(std::tuple<int, std::string>* error);
			std::string getDebugName() const;
			const char* getTypeName() const;

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
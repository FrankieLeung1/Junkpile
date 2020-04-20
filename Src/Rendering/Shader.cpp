#include "stdafx.h"
#include "Shader.h"
#include "../Resources/ResourceManager.h"
#include "../Framework/Framework.h"
#include "RenderingDevice.h"
#include "../Misc/Misc.h"
#include "../Files/FileManager.h"

using namespace Rendering;

Shader::Shader():
m_module(nullptr)
{

}

Shader::Shader(Type type, const char* code)
{
	setCode(type, code);
}

Shader::~Shader()
{
	freeModule();
}

std::string Shader::getCachePath() const
{
	const char* type = (m_type == Type::Vertex ? "vert" : "frag");
	return stringf("%sShaderCache/%x.%s", Framework::getResPath(), m_hash, type);
}

vk::ShaderModule Shader::getModule() const
{
	return m_module;
}

void Shader::setCode(Type type, const char* code)
{
	m_type = type;
	m_code = code;
	m_hash = generateHash(code, strlen(code));
}

bool Shader::compile(std::string* outError)
{
	using namespace Framework;
	ResourcePtr<Rendering::Device> device;
	ResourcePtr<FileManager> files;

	std::string cachePath = getCachePath();
	if (!files->exists(cachePath.c_str()))
	{
		std::stringstream error;
		std::stringstream cmd;
		const char* type = (m_type == Type::Vertex ? "vert" : "frag");
		cmd << getExternalPath() << "glslangValidator --stdin -S " << type << " -V -o \"" << cachePath << '\"';
		ProcessHandle* process = popen(cmd.str().c_str());
		if (process)
		{
			write(process, m_code.c_str(), m_code.size());
			closeWrite(process);

			int errorCode;
			while (!getExitCode(process, &errorCode))
				;

			if (errorCode != 0)
			{
				char buffer[1024] = { '\0' };
				read(process, buffer, sizeof(buffer) - 1);
				if (buffer[0] != '\0' && strcmp(buffer, "stdin\r\n") != 0)
					error << buffer;
			}
			pclose(process);
		}

		bool hasError = !error.str().empty();
		if (hasError && outError) *outError = error.str();
		if(hasError)
			return false;
	}

	// TODO: don't sync
	ResourcePtr<File> cacheFile(NewPtr, cachePath.c_str());
	freeModule();

	//LOG_IF_F(ERROR, !cacheFile, "failed to open shader cache %s\n", cachePath.c_str());

	vk::ShaderModuleCreateInfo info = {};
	info.codeSize = cacheFile->getSize();
	info.pCode = (uint32_t*)cacheFile->getContents();
	m_module = device->createObject(info);
	return true;
}

Shader::Type Shader::getType() const
{
	return m_type;
}

void Shader::freeModule()
{
	if (m_module)
	{
		ResourcePtr<Rendering::Device> device;
		//device->getDevice().destroyShaderModule(m_module);
		m_module = nullptr;
	}
}

void Shader::test()
{
	char vertexCode[] = R";(#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex{ vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
	Out.Color = aColor;
	Out.UV = aUV;
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
});";

	ResourcePtr<Shader> vertShader(NewPtr, Shader::Type::Vertex, vertexCode);
	std::size_t offset = 0;
	
	char pixelCode[] = R";(#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
    fColor = In.Color * texture(sTexture, In.UV.st);
}
);";

	ResourcePtr<Shader> fragShader(NewPtr, Shader::Type::Pixel, pixelCode);
}

Shader::ShaderLoader* Shader::createLoader(Type type, const char* code)
{
	return new ShaderLoader(type, code);
}

std::tuple<bool, std::size_t> Shader::getSharedHash(Type type, const char* code)
{
	return {true, generateHash(code, strlen(code)) };
}

Shader::ShaderLoader::ShaderLoader(Type type, const char* code):
m_type(type),
m_code(code)
{

}

Shader::ShaderLoader::~ShaderLoader()
{

}

Shader* Shader::ShaderLoader::load(std::tuple<int, std::string>* error)
{
	Shader* shader = new Shader(m_type, m_code.c_str());
	if (!shader->compile(&std::get<1>(*error)))
	{
		std::get<0>(*error) = -1;
		LOG_F(ERROR, "Failed to compile shader \"%s\"\n%s\n", getDebugName().c_str(), std::get<1>(*error).c_str());
		delete shader;
		shader = nullptr;
	}

	return shader;
}

std::string Shader::ShaderLoader::getDebugName() const
{
	return "shader";
}

const char* Shader::ShaderLoader::getTypeName() const
{
	return "shader";
}

Binding::Binding() {}
Binding::Binding(Type t, int i, vk::Format format, std::size_t offset) : m_type(t), m_index(i), m_format(format), m_offset(offset) {}
Binding::Binding(Type t, const char* n, vk::Format format, std::size_t offset) : m_type(t), m_name(n), m_format(format), m_offset(offset) {}
Binding::~Binding() {}
bool Binding::operator==(const Binding& other) const { return other.m_index == m_index && other.m_type == m_type && other.m_name == m_name; }
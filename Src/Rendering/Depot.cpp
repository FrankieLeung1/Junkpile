#include "stdafx.h"
#include "Depot.h"
#include "Shader.h"

using namespace Rendering;
Depot::Depot():
m_passVShader(EmptyPtr),
m_passFShader(EmptyPtr),
m_texVShader(EmptyPtr),
m_texFShader(EmptyPtr)
{

}

Depot::~Depot()
{
}

ResourcePtr<Rendering::Shader> Depot::getPassthroughVertexShader()
{
	if (!m_passVShader)
	{
		char vertexCode[] =
			"#version 450 core\n"
			"layout(location = 0) in vec3 aPos;\n"
			"layout(location = 1) in vec3 aColour;\n"
			"layout(push_constant) uniform PushConsts{ mat4 vp; } pushConsts;\n"
			//"out gl_PerVertex{ vec4 gl_Position; vec3 gl_Colour; };\n"
			"layout(location = 0) out vec3 outColour;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = pushConsts.vp * vec4(aPos.xyz, 1.0);\n"
			"	outColour = aColour;\n"
			"}";

		m_passVShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Vertex, vertexCode);
	}

	return m_passVShader;
}

ResourcePtr<Rendering::Shader> Depot::getPassthroughFragmentShader()
{
	if (!m_passFShader)
	{
		char pixelCode[] =
			"#version 450 core\n"
			"layout(location = 0) in vec3 fragColour;\n"
			"layout(location = 0) out vec4 fColor; \n"
			"void main()\n"
			"{\n"
			"	fColor = vec4(fragColour, 1.0);\n"
			"}";

		m_passFShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);
	}

	return m_passFShader;
}

ResourcePtr<Rendering::Shader> Depot::getTexturedVertexShader()
{
	if (!m_texVShader)
	{
		char vertexCode[] =
			"#version 450 core\n"
			"layout(location = 0) in vec3 aPos;\n"
			"layout(location = 1) in vec2 aUV;\n"
			"layout(binding = 0) uniform UBO\n"
			"{\n"
			"	mat4 projection;\n"
			"	mat4 model;\n"
			"} ubo;\n"
			"out gl_PerVertex{ vec4 gl_Position; };\n"
			"layout(location = 0) out vec2 UV;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = ubo.projection * ubo.model * vec4(aPos.xyz, 1.0);\n"
			//"	gl_Position = pushConsts.vp * vec4(aPos.xyz, 1.0);\n"
			"	UV = aUV;\n"
			"}";

		m_texVShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Vertex, vertexCode);
	}

	return m_texVShader;
}

ResourcePtr<Rendering::Shader> Depot::getTexturedFragmentShader()
{
	if (!m_texFShader)
	{
		char pixelCode[] =
			"#version 450 core\n"
			"layout(location = 0) in vec2 UV;\n"
			"layout(location = 0) out vec4 fColor; \n"
			"layout(binding = 1) uniform sampler2D sTexture;\n"
			"void main()\n"
			"{\n"
			"	fColor = texture(sTexture, UV.st);\n"
			"}";

		m_texFShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);
	}

	return m_texFShader;
}
#include "stdafx.h"
#include "Depot.h"
#include "Shader.h"

using namespace Rendering;
Depot::Depot():
m_passVShader(EmptyPtr),
m_passFShader(EmptyPtr)
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
			"layout(location = 0) in vec2 aPos;\n"
			//"layout(location = 1) in vec2 aUV;\n"
			//"layout(push_constant) uniform PushConsts{ mat4 vp; } pushConsts;\n"
			"out gl_PerVertex{ vec4 gl_Position; };\n"
			//"layout(location = 0) out vec2 UV;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = vec4(aPos.xy, 0.0, 1.0);\n"
			//"	UV = aUV;\n"
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
			//"layout(location = 0) in vec2 UV;\n"
			"layout(location = 0) out vec4 fColor; \n"
			"void main()\n"
			"{\n"
			"	fColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}";

		m_passFShader = ResourcePtr<Rendering::Shader>(NewPtr, Rendering::Shader::Type::Pixel, pixelCode);
	}

	return m_passFShader;
}
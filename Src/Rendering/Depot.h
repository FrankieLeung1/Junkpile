#pragma once

#include "../Resources/ResourceManager.h"
namespace Rendering {

	class Shader;
	class Texture;
	class Depot : public SingletonResource<Depot>
	{
	public:
		Depot();
		~Depot();

		ResourcePtr<Rendering::Shader> getPassthroughVertexShader();
		ResourcePtr<Rendering::Shader> getPassthroughFragmentShader();

		ResourcePtr<Rendering::Shader> getTexturedVertexShader();
		ResourcePtr<Rendering::Shader> getTexturedFragmentShader();

	protected:
		ResourcePtr<Rendering::Shader> m_passVShader, m_passFShader;
		ResourcePtr<Rendering::Shader> m_texVShader, m_texFShader;
	};

}
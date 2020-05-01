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

	protected:
		ResourcePtr<Rendering::Shader> m_passVShader;
		ResourcePtr<Rendering::Shader> m_passFShader;
	};

}
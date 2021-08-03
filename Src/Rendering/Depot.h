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

		ResourcePtr<Rendering::Shader> getBWFragmentShader();
		ResourcePtr<Rendering::Shader> getInvertFragmentShader();

	protected:
		ResourcePtr<Rendering::Shader> m_passVShader, m_passFShader;
		ResourcePtr<Rendering::Shader> m_texVShader, m_texFShader;
		ResourcePtr<Rendering::Shader> m_bwFShader, m_invertFShader;
	};

	template<ResourcePtr<Rendering::Shader>(Depot::*VertexShaderFunc)(), ResourcePtr<Rendering::Shader>(Depot::* FragmentShaderFunc)()>
	struct BaseVertex
	{
		static ResourcePtr<Rendering::Shader> getVertexShader();
		static ResourcePtr<Rendering::Shader> getFragmentShader();
	};

	struct ColouredVert : public BaseVertex<&Depot::getPassthroughVertexShader, &Depot::getPassthroughFragmentShader>
	{
		glm::vec3 m_position, m_colour;
	};

	// ----------------------- IMPLEMENTATION ----------------------- 
	template<ResourcePtr<Rendering::Shader>(Depot::* VertexShaderFunc)(), ResourcePtr<Rendering::Shader>(Depot::* FragmentShaderFunc)()>
	ResourcePtr<Rendering::Shader> BaseVertex<VertexShaderFunc, FragmentShaderFunc>::getVertexShader()
	{
		return (ResourcePtr<Depot>()->*VertexShaderFunc)();
	}

	template<ResourcePtr<Rendering::Shader>(Depot::* VertexShaderFunc)(), ResourcePtr<Rendering::Shader>(Depot::* FragmentShaderFunc)()>
	ResourcePtr<Rendering::Shader> BaseVertex<VertexShaderFunc, FragmentShaderFunc>::getFragmentShader()
	{
		return (ResourcePtr<Depot>()->*FragmentShaderFunc)();
	}

}
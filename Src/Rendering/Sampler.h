#pragma once

namespace Rendering
{
	class Sampler
	{
	public:
		Sampler();
		~Sampler();

	protected:
		vk::Sampler m_sampler;
	};
}
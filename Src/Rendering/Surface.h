#pragma once

namespace Rendering
{
	class Surface
	{
	public:
		virtual void setSurfaceImage(vk::Image image, VmaAllocation memory) =0;
		virtual vk::Image getSurfaceImage() =0;
	};
}
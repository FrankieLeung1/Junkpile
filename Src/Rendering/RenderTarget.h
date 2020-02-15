#pragma once

namespace Rendering
{
	class RenderTarget
	{
	public:
		virtual void setVkImageRT(vk::Image image, VmaAllocation memory) =0;
		virtual vk::Image getVkImageRT() =0;
	};
}
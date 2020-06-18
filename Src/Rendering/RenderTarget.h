#pragma once

namespace Rendering
{
	class RenderTarget
	{
	public:
		enum Type { Colour, DepthStencil };
		RenderTarget(Type, int width, int height);
		RenderTarget(vk::Format, vk::ImageTiling tiling, vk::FormatFeatureFlags features, int width, int height);
		RenderTarget(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, int width, int height);
		~RenderTarget();

		vk::Format getFormat() const;
		vk::Image getImage();
		vk::ImageView getView();

		vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
		bool hasStencil(vk::Format) const;

	protected:
		std::vector<vk::Format> getFormat(Type) const;
		vk::ImageTiling getTiling(Type) const;
		vk::FormatFeatureFlags getFeatures(Type) const;

	protected:
		vk::Image m_image;
		VmaAllocation m_allocation;
		vk::ImageView m_view;

		vk::Format m_format;
	};
}
#pragma once

namespace Rendering
{
	// is buffer a resource?
	class Buffer
	{
	public:
		struct Format
		{
			vk::Format m_format;
			std::size_t m_size;
			Format(vk::Format format, std::size_t size) : m_format(format), m_size(size) {}
		};

	public:
		enum Type {Vertex, Index, Uniform};
		enum Usage {Static, Mapped};
		Buffer(Type, Usage, std::size_t size);
		~Buffer();

		Type getType() const;
		Usage getUsage() const;

		void* map();
		void unmap();

		void grow(std::size_t minSize);

		void setFormat(std::vector<Format>&&, std::size_t stride);
		const std::vector<Format>& getFormat() const;
		
		std::size_t getStride() const;
		std::size_t getSize() const;
		vk::Buffer getVkBuffer() const;

	protected:
		void recreate(Type, Usage, std::size_t size);

	protected:
		Type m_type;
		Usage m_usage;
		std::size_t m_size;
		ResourcePtr<Rendering::Device> m_device;
		vk::Buffer m_buffer;
		VmaAllocation m_allocation;
		std::vector<Format> m_format;
		std::size_t m_stride;
	};
}
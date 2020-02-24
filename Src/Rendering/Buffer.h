#pragma once

namespace Rendering
{
	// is buffer a resource?
	class Buffer
	{
	public:
		enum Type {Vertex, Index, Uniform};
		enum Usage {Static, Mapped};
		Buffer(Type, Usage, std::size_t size);
		~Buffer();

		Type getType() const;
		Usage getUsage() const;

		void* map();
		void unmap();

		std::size_t getSize() const;

	protected:
		Type m_type;
		Usage m_usage;
		std::size_t m_size;
		ResourcePtr<Rendering::Device> m_device;
		vk::Buffer m_buffer;
		VmaAllocation m_allocation;
	};
}
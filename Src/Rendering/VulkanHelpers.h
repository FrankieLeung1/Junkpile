#pragma once

namespace Rendering
{
	void checkVkResult(VkResult);
	void checkVkResult(vk::Result);
	uint32_t findMemoryType(VkPhysicalDevice device, VkMemoryPropertyFlags properties, uint32_t type_bits);
}
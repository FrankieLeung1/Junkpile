#include "stdafx.h"
#include "VulkanHelpers.h"

void Rendering::checkVkResult(VkResult r)
{
	const char* msg = nullptr;
	switch (r)
	{
	case VK_SUCCESS: return;
	case VK_NOT_READY: msg = "A fence or query has not yet completed"; break;
	case VK_TIMEOUT: msg = "A wait operation has not completed in the specified time"; break;
	case VK_EVENT_SET: msg = "An event is signaled"; break;
	case VK_EVENT_RESET: msg = "An event is unsignaled"; break;
	case VK_INCOMPLETE: msg = "A return array was too small for the result"; break;
	case VK_SUBOPTIMAL_KHR: msg = "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully."; break;
	case VK_ERROR_OUT_OF_HOST_MEMORY: msg = "A host memory allocation has failed."; break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: msg = "A device memory allocation has failed."; break;
	case VK_ERROR_INITIALIZATION_FAILED: msg = "Initialization of an object could not be completed for implementation - specific reasons."; break;
	case VK_ERROR_DEVICE_LOST: msg = "The logical or physical device has been lost.See Lost Device"; break;
	case VK_ERROR_MEMORY_MAP_FAILED: msg = "Mapping of a memory object has failed."; break;
	case VK_ERROR_LAYER_NOT_PRESENT: msg = "A requested layer is not present or could not be loaded."; break;
	case VK_ERROR_EXTENSION_NOT_PRESENT: msg = "A requested extension is not supported."; break;
	case VK_ERROR_FEATURE_NOT_PRESENT: msg = "A requested feature is not supported."; break;
	case VK_ERROR_INCOMPATIBLE_DRIVER: msg = "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation - specific reasons."; break;
	case VK_ERROR_TOO_MANY_OBJECTS: msg = "Too many objects of the type have already been created."; break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED: msg = "A requested format is not supported on this device."; break;
	case VK_ERROR_FRAGMENTED_POOL: msg = "A pool allocation has failed due to fragmentation of the pool’s memory.This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation.This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation."; break;
	case VK_ERROR_SURFACE_LOST_KHR: msg = "A surface is no longer available."; break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: msg = "The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again."; break;
	case VK_ERROR_OUT_OF_DATE_KHR: msg = "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail.Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface."; break;
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: msg = "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image."; break;
	case VK_ERROR_INVALID_SHADER_NV: msg = "One or more shaders failed to compile or link.More details are reported back to the application via https ://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_EXT_debug_report if enabled."; break;
	case VK_ERROR_OUT_OF_POOL_MEMORY: msg = "A pool memory allocation has failed.This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation.If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead."; break;
	case VK_ERROR_INVALID_EXTERNAL_HANDLE: msg = "An external handle is not a valid handle of the specified type."; break;
	case VK_ERROR_FRAGMENTATION_EXT: msg = "A descriptor pool creation has failed due to fragmentation."; break;
	case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: msg = "A buffer creation failed because the requested address is not available."; break;
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: msg = "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full - screen access.This may occur due to implementation - dependent reasons, outside of the application’s control."; break;
	}
	LOG_F(FATAL, "Vulkan Error: %s\n", msg);

	switch (r)
	{
	case VK_NOT_READY: msg = "A fence or query has not yet completed"; break;
	case VK_TIMEOUT: msg = "A wait operation has not completed in the specified time"; break;
	case VK_EVENT_SET: msg = "An event is signaled"; break;
	case VK_EVENT_RESET: msg = "An event is unsignaled"; break;
	case VK_INCOMPLETE: msg = "A return array was too small for the result"; break;
	case VK_SUBOPTIMAL_KHR: msg = "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully."; break;
	}
	LOG_F(WARNING, "Vulkan Warning: %s\n", msg);
}

void Rendering::checkVkResult(vk::Result r)
{
	checkVkResult((VkResult)(int)r);
}

uint32_t Rendering::findMemoryType(VkPhysicalDevice device, VkMemoryPropertyFlags properties, uint32_t type_bits)
{
	VkPhysicalDeviceMemoryProperties prop;
	vkGetPhysicalDeviceMemoryProperties(device, &prop);
	for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
		if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
			return i;

	return 0xFFFFFFFF; // Unable to find memoryType
}
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
#define _SCL_SECURE_NO_WARNINGS

#define PY_SSIZE_T_CLEAN
#ifdef _DEBUG
	#undef _DEBUG
	#include <python.h>
	#include <structmember.h>
	#define _DEBUG
#else
	#include <python.h>
	#include <structmember.h>
#endif

#include "lua/src/lua.hpp"
#include "loguru/loguru.hpp"
#include "imgui/imgui.h"
#include "FileWatcher/FileWatcher.h"
#include "vk_mem_alloc.h"
#include "Fossilize/fossilize.hpp"

#include <algorithm>
#include <chrono>
#include <forward_list>
#include <array>
#include <cstddef>
#include <tuple>
#include <memory>
#include <vector>
#include <set>
#include <forward_list>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <future>
#include <queue>
#include <bitset>
#include <random>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "glm/glm/glm.hpp"
#include "glm/glm/ext.hpp"
#include <vulkan/vulkan.h>

#define VULKAN_HPP_ASSERT CHECK_F
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

#include "../Framework/VulkanFramework.h"
typedef VulkanFramework FrameworkClass;
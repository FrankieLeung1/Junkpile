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

#include <algorithm>
#include <chrono>
#include <forward_list>
#include <array>
#include <cstddef>
#include <tuple>
#include <memory>
#include <vector>
#include <array>
#include <set>
#include <list>
#include <forward_list>
#include <stack>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <thread>
#include <future>
#include <queue>
#include <bitset>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <locale>
#include <utility>
#include <codecvt>
#include <sys/stat.h>
#include <time.h>
#include <WinSock2.h>
#include <Windows.h>

#include "lua/src/lua.hpp"
#include "loguru/loguru.hpp"
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "implot/implot.h"
#include "imgui/addons/imguidatechooser/imguidatechooser.h"
#include "imgui/addons/imguinodegrapheditor/imguinodegrapheditor.h"
#include "FileWatcher/FileWatcher.h"
#include "vk_mem_alloc.h"
#include "Fossilize/fossilize.hpp"
#include "openfbx/ofbx.h"
#include <cstdio>
#include "CImg.h"

#define LODEPNG_NO_COMPILE_DISK
#include "lodepng/lodepng.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm/glm.hpp"
#include "glm/glm/ext.hpp"
#include <vulkan/vulkan.h>

#define VULKAN_HPP_ASSERT (void)
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

#include "../Framework/VulkanFramework.h"
#include "../Managers/DebugManager.h"
#include "../Misc/CallStack.h"

typedef VulkanFramework FrameworkClass;
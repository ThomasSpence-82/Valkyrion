#pragma once

// Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// C++ standard library
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <chrono>
#include <thread>

// Vulkan
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT
#include <vulkan/vulkan.hpp>

// Engine exports
#ifdef VALKYRION_ENGINE_EXPORTS
    #define VK_API __declspec(dllexport)
#else
    #define VK_API __declspec(dllimport)
#endif

// Engine configuration
#define VK_ENABLE_VALIDATION_LAYERS _DEBUG

// Engine forward declarations
namespace Valkyrion
{
    class Application;
    class Window;
    class VulkanContext;
}

#pragma once

#include "pch.h"

// VK_API is defined in pch.h

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define VK_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR == 1
        #define VK_PLATFORM_IOS_SIMULATOR
    #elif TARGET_OS_IPHONE == 1
        #define VK_PLATFORM_IOS
    #elif TARGET_OS_MAC == 1
        #define VK_PLATFORM_MACOS
    #else
        #error "Unknown Apple platform"
    #endif
#elif defined(__ANDROID__)
    #define VK_PLATFORM_ANDROID
#elif defined(__linux__)
    #define VK_PLATFORM_LINUX
#else
    #error "Unknown platform!"
#endif

// Debug break
#if defined(VK_DEBUG) || defined(_DEBUG)
    #if defined(VK_PLATFORM_WINDOWS)
        #define VK_DEBUGBREAK() __debugbreak()
    #else
        #include <signal.h>
        #define VK_DEBUGBREAK() raise(SIGTRAP)
    #endif
    #define VK_ENABLE_ASSERTS
#else
    #define VK_DEBUGBREAK()
#endif

// Assertions
#ifdef VK_ENABLE_ASSERTS
    #define VK_ASSERT(x, ...) { if(!(x)) { VKLOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); VK_DEBUGBREAK(); } }
    #define VK_CORE_ASSERT(x, ...) { if(!(x)) { VKLOG_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); VK_DEBUGBREAK(); } }
#else
    #define VK_ASSERT(x, ...)
    #define VK_CORE_ASSERT(x, ...)
#endif

// Include Log.h for logging macros
#include "Valkyrion/Core/Log.h"

// Smart pointers
template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// Bit operations
#define VK_BIT(x) (1 << x)

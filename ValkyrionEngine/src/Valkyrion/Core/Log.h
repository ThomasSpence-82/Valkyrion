#pragma once

// Use the precompiled spdlog library
#ifndef SPDLOG_HEADER_ONLY
#define SPDLOG_HEADER_ONLY 0
#endif

#define SPDLOG_COMPILED_LIB

#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

namespace Valkyrion {

class Log
{
public:
    static void Init() {
        std::call_once(s_InitFlag, &Log::InitImpl);
    }
    
    inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { 
        Init();
        return s_CoreLogger; 
    }
    
    inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { 
        Init();
        return s_ClientLogger; 
    }

private:
    static void InitImpl();
    static std::once_flag s_InitFlag;
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Valkyrion

// Core log macros
#define VKLOG_CORE_TRACE(...)    ::Valkyrion::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VKLOG_CORE_INFO(...)     ::Valkyrion::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VKLOG_CORE_WARN(...)     ::Valkyrion::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VKLOG_CORE_ERROR(...)    ::Valkyrion::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VKLOG_CORE_CRITICAL(...) ::Valkyrion::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define VKLOG_TRACE(...)         ::Valkyrion::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VKLOG_INFO(...)          ::Valkyrion::Log::GetClientLogger()->info(__VA_ARGS__)
#define VKLOG_WARN(...)          ::Valkyrion::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VKLOG_ERROR(...)         ::Valkyrion::Log::GetClientLogger()->error(__VA_ARGS__)
#define VKLOG_CRITICAL(...)      ::Valkyrion::Log::GetClientLogger()->critical(__VA_ARGS__)

// Backwards compatibility
#define VK_TRACE    VKLOG_TRACE
#define VK_INFO     VKLOG_INFO
#define VK_WARN     VKLOG_WARN
#define VK_ERROR    VKLOG_ERROR
#define VK_CRITICAL VKLOG_CRITICAL

#define VK_CORE_TRACE    VKLOG_CORE_TRACE
#define VK_CORE_INFO     VKLOG_CORE_INFO
#define VK_CORE_WARN     VKLOG_CORE_WARN
#define VK_CORE_ERROR    VKLOG_CORE_ERROR
#define VK_CORE_CRITICAL VKLOG_CORE_CRITICAL

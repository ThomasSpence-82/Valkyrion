#define SPDLOG_HEADER_ONLY 0
#define SPDLOG_COMPILED_LIB
#include "Valkyrion/Core/Log.h"

#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Valkyrion {

std::once_flag Log::s_InitFlag;
std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::InitImpl()
{
    try {
        // First, try to get existing loggers
        auto core_logger = spdlog::get("VALKYRION");
        auto client_logger = spdlog::get("APP");
        
        if (!core_logger) {
            // Create new core logger
            s_CoreLogger = spdlog::stdout_color_mt("VALKYRION");
        } else {
            // Use existing logger
            s_CoreLogger = core_logger;
        }
        
        if (!client_logger) {
            // Create new client logger
            s_ClientLogger = spdlog::stdout_color_mt("APP");
        } else {
            // Use existing logger
            s_ClientLogger = client_logger;
        }
        
        // Configure log levels and pattern
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_CoreLogger->set_level(spdlog::level::trace);
        s_ClientLogger->set_level(spdlog::level::trace);
        
        // Log initialization message directly to avoid recursive initialization
        s_CoreLogger->info("Logging system initialized");
    }
    catch (const spdlog::spdlog_ex& ex) {
        // If we get here, there was an error initializing the logger
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        throw; // Re-throw the exception
    }
}

} // namespace Valkyrion

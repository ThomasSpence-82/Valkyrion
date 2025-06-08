#include "pch.h"
#include "SandboxApp.h"
#include "ECSTest.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>

namespace Valkyrion {

SandboxApp::SandboxApp()
{
    spdlog::info("Sandbox application created");
    
    try {
        // Run ECS test
        ECSTest::Run();
    }
    catch (const std::exception& e) {
        spdlog::error("Error in SandboxApp: {}", e.what());
    }
}

void SandboxApp::OnUpdate()
{
    // Update game logic here
}

void SandboxApp::OnRender()
{
    // Render game here
}

} // namespace Valkyrion

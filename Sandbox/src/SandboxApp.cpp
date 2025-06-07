#include "pch.h"
#include "SandboxApp.h"

#include <spdlog/spdlog.h>

namespace Valkyrion {

SandboxApp::SandboxApp()
{
    spdlog::info("Sandbox application created");
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

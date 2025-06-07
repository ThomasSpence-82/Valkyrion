#include "Valkyrion/Core/Application.h"

class SandboxApp : public Valkyrion::Application
{
public:
    SandboxApp()
        : Application("Valkyrion Sandbox")
    {
    }
    
    ~SandboxApp() override = default;
    
    void OnUpdate() override
    {
        // Update game logic here
    }
    
    void OnRender() override
    {
        // Render game objects here
    }
};

// Implement the CreateApplication function that returns a new SandboxApp
Valkyrion::Application* Valkyrion::CreateApplication()
{
    return new SandboxApp();
}

// Main entry point
int main(int /*argc*/, char** /*argv*/)
{
    try {
        auto app = Valkyrion::CreateApplication();
        app->Run();
        delete app;
        return 0;
    }
    catch (const std::exception& e) {
        VKLOG_CORE_CRITICAL("Unhandled exception: {}", e.what());
        return -1;
    }
}

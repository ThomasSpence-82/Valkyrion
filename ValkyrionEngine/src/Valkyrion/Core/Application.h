#pragma once

#include "Core.h"
#include "Window.h"

namespace Valkyrion {

// Forward declarations
class VulkanContext;

class Application
{
public:
    Application(const std::string& name = "Valkyrion App");
    virtual ~Application();

    void Run();
    
    // Application events
    virtual void OnInitialize() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}
    virtual void OnShutdown() {}

    static Application& Get() { return *s_Instance; }
    Window& GetWindow() { return *m_Window; }

private:
    void Initialize();
    void Shutdown();

private:
    std::string m_Name;
    bool m_Running = false;
    float m_LastFrameTime = 0.0f;
    
    Scope<Window> m_Window;
    Scope<VulkanContext> m_VulkanContext;

    static Application* s_Instance;
};

// To be defined in CLIENT
Application* CreateApplication();

} // namespace Valkyrion

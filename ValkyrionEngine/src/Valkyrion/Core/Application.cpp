#include "Valkyrion/Core/Application.h"
#include "Valkyrion/Core/Window.h"
#include "Valkyrion/Core/Log.h"
#include "Valkyrion/Renderer/Vulkan/VulkanContext.h"

namespace Valkyrion {

Application* Application::s_Instance = nullptr;

Application::Application(const std::string& name)
    : m_Name(name)
{
    VK_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;
    
    // Initialize logging
    Log::Init();
    VKLOG_INFO("Initializing application: {0}", name);
    
    // Initialize systems
    Initialize();
}

Application::~Application()
{
    Shutdown();
}

void Application::Initialize()
{
    // Create window
    VKLOG_INFO("Creating window: {0} ({1}x{2})", m_Name, 1600, 900);
    WindowProps props;
    props.Title = m_Name;
    props.Width = 1600;
    props.Height = 900;
    
    m_Window = CreateScope<Window>(props);
    
    // Initialize Vulkan
    m_VulkanContext = CreateScope<VulkanContext>(*m_Window);
    
    // Call user initialization
    OnInitialize();
    
    VKLOG_INFO("Initialized application '{0}'", m_Name);
}

void Application::Shutdown()
{
    if (!m_Running)
        return;
        
    VKLOG_INFO("Shutting down application '{0}'...", m_Name);
    
    // Call user shutdown
    OnShutdown();
    
    // Shutdown systems
    m_VulkanContext.reset();
    m_Window.reset();
    
    s_Instance = nullptr;
    m_Running = false;
}

void Application::Run()
{
    m_Running = true;
    
    VKLOG_INFO("Starting application loop...");
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    // Main loop
    while (m_Running && !m_Window->ShouldClose())
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        [[maybe_unused]] float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Update window
        m_Window->OnUpdate();
        
        // Update application
        OnUpdate();
        
        // Render frame
        OnRender();
        
        // Present
        m_VulkanContext->SwapBuffers();
    }
    
    // Wait for device to finish before cleanup
    vkDeviceWaitIdle(m_VulkanContext->GetDevice());
}

} // namespace Valkyrion

#include "Valkyrion/Core/Window.h"
#include "Valkyrion/Core/Log.h"

// GLFW
#include <GLFW/glfw3.h>

namespace Valkyrion {

static void GLFWErrorCallback(int error, const char* description)
{
    VKLOG_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

Window::Window(const WindowProps& props)
{
    Init(props);
}

Window::~Window()
{
    Shutdown();
}

void Window::Init(const WindowProps& props)
{
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    
    VKLOG_INFO("Creating window '{0}' ({1}x{2})", props.Title, props.Width, props.Height);
    
    // Initialize GLFW
    if (!glfwInit())
    {
        VKLOG_CRITICAL("Could not initialize GLFW!");
        return;
    }
    
    // Set error callback
    glfwSetErrorCallback(GLFWErrorCallback);
    
    // Configure GLFW for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Create window
    m_Window = glfwCreateWindow(
        static_cast<int>(props.Width),
        static_cast<int>(props.Height),
        m_Data.Title.c_str(),
        nullptr,
        nullptr
    );
    
    if (!m_Window)
    {
        VKLOG_CRITICAL("Failed to create GLFW window!");
        glfwTerminate();
        return;
    }
    
    // Set window user pointer
    glfwSetWindowUserPointer(m_Window, this);
    
    // Enable VSync by default
    SetVSync(true);
    
    VKLOG_INFO("Window created successfully!");
}

void Window::Shutdown()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    
    glfwTerminate();
}

void Window::OnUpdate()
{
    glfwPollEvents();
}

void Window::SetVSync(bool enabled)
{
    // VSync is handled by the swapchain in Vulkan
    m_Data.VSync = enabled;
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window);
}

Scope<Window> Window::Create(const WindowProps& props)
{
    return CreateScope<Window>(props);
}

} // namespace Valkyrion

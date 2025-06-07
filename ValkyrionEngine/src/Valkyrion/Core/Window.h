#pragma once

#include "Core.h"

struct GLFWwindow;

namespace Valkyrion {

struct WindowProps
{
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    
    WindowProps(
        const std::string& title = "Valkyrion Engine",
        uint32_t width = 1600,
        uint32_t height = 900
    ) : Title(title), Width(width), Height(height) {}
};

class Window
{
public:
    Window(const WindowProps& props);
    ~Window();
    
    void OnUpdate();
    
    // Getters
    uint32_t GetWidth() const { return m_Data.Width; }
    uint32_t GetHeight() const { return m_Data.Height; }
    GLFWwindow* GetNativeWindow() const { return m_Window; }
    
    // Window attributes
    void SetVSync(bool enabled);
    bool IsVSync() const { return m_Data.VSync; }
    
    bool ShouldClose() const;
    
    static Scope<Window> Create(const WindowProps& props = WindowProps());
    
private:
    void Init(const WindowProps& props);
    void Shutdown();
    
private:
    GLFWwindow* m_Window;
    
    struct WindowData
    {
        std::string Title;
        uint32_t Width, Height;
        bool VSync;
    };
    
    WindowData m_Data;
};

} // namespace Valkyrion

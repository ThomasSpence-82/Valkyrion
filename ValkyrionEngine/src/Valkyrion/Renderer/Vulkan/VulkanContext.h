#pragma once

#include "Valkyrion/Core/Window.h"

#include <vulkan/vulkan.hpp>

namespace Valkyrion {

class VulkanContext
{
public:
    VulkanContext(Window& window);
    ~VulkanContext();
    
    void Init();
    void Shutdown();
    
    void SwapBuffers();
    
    // Getters
    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[0]; } // TODO: Proper command buffer management
    
    static VulkanContext* Get() { return s_Instance; }
    
private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    
    bool CheckValidationLayerSupport();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    
    // Helper functions for swap chain
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    
    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        int presentFamily = -1;
        
        bool isComplete() {
            return graphicsFamily >= 0 && presentFamily >= 0;
        }
    };
    
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    
private:
    Window& m_Window;
    
    // Vulkan objects
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    
    // Queue family indices
    uint32_t m_GraphicsQueueFamily = UINT32_MAX;
    uint32_t m_PresentQueueFamily = UINT32_MAX;
    
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapChainImages;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;
    std::vector<VkImageView> m_SwapChainImageViews;
    
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    
    // Render pass and framebuffers
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    
    // Sync objects
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    size_t m_CurrentFrame = 0;
    
    // Validation layers
    const std::vector<const char*> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    // Enable validation layers in debug builds
    #ifdef VK_DEBUG
    const bool m_EnableValidationLayers = true;
    #else
    const bool m_EnableValidationLayers = false;
    #endif
    
    // Device extensions
    const std::vector<const char*> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    static VulkanContext* s_Instance;
};

} // namespace Valkyrion

#include "Valkyrion/Renderer/Vulkan/VulkanContext.h"
#include "Valkyrion/Core/Window.h"
#include "Valkyrion/Core/Core.h"
#include "Valkyrion/Core/Log.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <set>
#include <stdexcept>

// Constants
const int MAX_FRAMES_IN_FLIGHT = 2;

// Validation layers
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Device extensions
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Macro to check Vulkan results
#define VK_CHECK_RESULT(f) \
    do { \
        VkResult result = (f); \
        if (result != VK_SUCCESS) { \
            VKLOG_CORE_ERROR("Vulkan error in {}: {}", #f, static_cast<int>(result)); \
            throw std::runtime_error("Vulkan error!"); \
        } \
    } while (0)

namespace Valkyrion {

// Static member initialization
VulkanContext* VulkanContext::s_Instance = nullptr;

VulkanContext::VulkanContext(Window& window)
    : m_Window(window)
{
    VK_ASSERT(!s_Instance, "VulkanContext already exists!");
    VKLOG_CORE_INFO("Creating Vulkan instance...");
    
    // Initialize logging if not already done
    Log::Init();
    s_Instance = this;
    Init();
}

VulkanContext::~VulkanContext()
{
    if (m_Device != VK_NULL_HANDLE) {
        // Wait for the device to finish all operations
        vkDeviceWaitIdle(m_Device);
        
        // Clean up framebuffers
        for (auto framebuffer : m_SwapChainFramebuffers) {
            vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
        }
        
        // Clean up render pass
        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        }
        
        // Clean up synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            }
            if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            }
            if (m_InFlightFences[i] != VK_NULL_HANDLE) {
                vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
            }
        }
        
        // Clean up command pool
        if (m_CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        }
        
        // Clean up swap chain image views
        for (auto imageView : m_SwapChainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_Device, imageView, nullptr);
            }
        }
        
        // Clean up swap chain
        if (m_SwapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
        }
    }
    
    // Clean up device
    if (m_Device) {
        vkDestroyDevice(m_Device, nullptr);
    }
    
    // Clean up debug messenger
    if (m_EnableValidationLayers) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_Instance, m_DebugMessenger, nullptr);
        }
    }
    
    // Clean up surface
    if (m_Surface) {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }
    
    // Clean up instance
    if (m_Instance) {
        vkDestroyInstance(m_Instance, nullptr);
    }
    
    VKLOG_CORE_INFO("Vulkan context destroyed successfully!");
}

void VulkanContext::Init()
{
    VKLOG_INFO("Initializing Vulkan context...");
    
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
    
    VKLOG_CORE_INFO("Vulkan context initialized successfully!");
}

void VulkanContext::Shutdown()
{
    vkDeviceWaitIdle(m_Device);
    
    // Cleanup swap chain
    for (auto imageView : m_SwapChainImageViews) {
        vkDestroyImageView(m_Device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    
    // Cleanup framebuffers
    for (auto framebuffer : m_SwapChainFramebuffers) {
        vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }
    
    // Cleanup render pass
    if (m_RenderPass) {
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    }
    
    // Cleanup sync objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
    }
    
    // Cleanup command pool
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    
    // Cleanup device
    vkDestroyDevice(m_Device, nullptr);
    
    // Cleanup debug messenger
    if (m_EnableValidationLayers) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_Instance, m_DebugMessenger, nullptr);
        }
    }
    
    // Cleanup surface and instance
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
    
    s_Instance = nullptr;
}

void VulkanContext::SwapBuffers()
{
    // Wait for the previous frame to finish
    vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
    
    // Acquire the next image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_Device, 
        m_SwapChain, 
        UINT64_MAX, 
        m_ImageAvailableSemaphores[m_CurrentFrame], 
        VK_NULL_HANDLE, 
        &imageIndex
    );
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handle window resize
        // TODO: RecreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        VKLOG_CORE_ERROR("Failed to acquire swap chain image!");
        return;
    }
    
    // Only reset the fence if we are submitting work
    vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);
    
    // Reset command buffer
    vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);
    
    // Record command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    
    if (vkBeginCommandBuffer(m_CommandBuffers[m_CurrentFrame], &beginInfo) != VK_SUCCESS) {
        VKLOG_CORE_ERROR("Failed to begin recording command buffer!");
        return;
    }
    
    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_SwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_SwapChainExtent;
    
    // Clear color (dark gray)
    VkClearValue clearColor = {0.2f, 0.2f, 0.2f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(m_CommandBuffers[m_CurrentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Here you would record your draw commands
    // For now, we're just clearing the screen
    
    // End render pass
    vkCmdEndRenderPass(m_CommandBuffers[m_CurrentFrame]);
    
    if (vkEndCommandBuffer(m_CommandBuffers[m_CurrentFrame]) != VK_SUCCESS) {
        VKLOG_CORE_ERROR("Failed to record command buffer!");
        return;
    }
    
    // Submit the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];
    
    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        VKLOG_CORE_ERROR("Failed to submit draw command buffer!");
        return;
    }
    
    // Present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = { m_SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    
    result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handle swap chain recreation
        // TODO: RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        VKLOG_CORE_ERROR("Failed to present swap chain image!");
    }
    
    // Move to the next frame
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Helper functions for swap chain
VulkanContext::SwapChainSupportDetails VulkanContext::QuerySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    
    // Get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);
    
    // Get surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
    }
    
    // Get present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    // If the preferred format is not available, just return the first one
    return availableFormats[0];
}

VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // Prefer MAILBOX mode for triple buffering if available
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    
    // Fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)m_Window.GetNativeWindow(), &width, &height);
        
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        // Clamp the values to the allowed range
        actualExtent.width = (std::max)(
            capabilities.minImageExtent.width,
            (std::min)(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = (std::max)(
            capabilities.minImageExtent.height,
            (std::min)(capabilities.maxImageExtent.height, actualExtent.height));
            
        return actualExtent;
    }
}

// Helper functions for Vulkan initialization
bool VulkanContext::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    for (const char* layerName : m_ValidationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device)
{
    // Basic device properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Device features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Check for required queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    bool hasGraphicsQueue = false;
    bool hasPresentQueue = false;
    m_GraphicsQueueFamily = UINT32_MAX;
    m_PresentQueueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const auto& queueFamily = queueFamilies[i];
        
        // Check for graphics queue
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            hasGraphicsQueue = true;
            m_GraphicsQueueFamily = i;
        }

        // Check for presentation support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
        if (presentSupport) {
            hasPresentQueue = true;
            m_PresentQueueFamily = i;
        }

        if (hasGraphicsQueue && hasPresentQueue) {
            break;
        }
    }

    // Check for required extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // Check for swap chain support
    bool swapChainAdequate = false;
    if (requiredExtensions.empty()) {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

        swapChainAdequate = formatCount > 0 && presentModeCount > 0;
    }

    return hasGraphicsQueue && hasPresentQueue && requiredExtensions.empty() && swapChainAdequate;
}

// Vulkan initialization methods
void VulkanContext::CreateInstance()
{
    if (m_EnableValidationLayers) {
        if (!CheckValidationLayerSupport()) {
            VKLOG_CORE_ERROR("Validation layers requested, but not available!");
            VK_CORE_ASSERT(false, "Validation layers requested, but not available!");
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Valkyrion Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Valkyrion";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (m_EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance));
    VKLOG_CORE_INFO("Vulkan instance created successfully!");
}

void VulkanContext::SetupDebugMessenger()
{
    if (!m_EnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VK_CHECK_RESULT(func(m_Instance, &createInfo, nullptr, &m_DebugMessenger));
    } else {
        VK_CORE_ERROR("Failed to set up debug messenger!");
    }
    VKLOG_CORE_INFO("Vulkan debug messenger created successfully!");
}

void VulkanContext::CreateSurface()
{
    VK_CHECK_RESULT(glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_Window.GetNativeWindow(), nullptr, &m_Surface));
    VKLOG_CORE_INFO("Vulkan surface created successfully!");
}

void VulkanContext::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        VK_CORE_ERROR("Failed to find GPUs with Vulkan support!");
        VK_CORE_ASSERT(false, "No Vulkan compatible GPU found!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            m_PhysicalDevice = device;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        VK_CORE_ERROR("Failed to find a suitable GPU!");
        VK_CORE_ASSERT(false, "No suitable GPU found!");
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
    VKLOG_CORE_INFO("Selected physical device: {0}", deviceProperties.deviceName);
}

void VulkanContext::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {m_GraphicsQueueFamily, m_PresentQueueFamily};
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (m_EnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device));

    // Get the queue handles
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);

    VKLOG_CORE_INFO("Logical device created successfully!");
}

void VulkanContext::CreateSwapChain()
{
    // Query swap chain support
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    m_SwapChainExtent = ChooseSwapExtent(swapChainSupport.capabilities);

    // Determine the number of images in the swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && 
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Create the swap chain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_SwapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {m_GraphicsQueueFamily, m_PresentQueueFamily};
    if (m_GraphicsQueueFamily != m_PresentQueueFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));

    // Retrieve swap chain images
    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
    m_SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());

    m_SwapChainImageFormat = surfaceFormat.format;
    
    VKLOG_CORE_INFO("Swap chain created successfully!");
}

void VulkanContext::CreateImageViews()
{
    m_SwapChainImageViews.resize(m_SwapChainImages.size());
    
    for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_SwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_SwapChainImageFormat;
        
        // Default component mapping
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        // Subresource range describes what the image's purpose is
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        VK_CHECK_RESULT(vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapChainImageViews[i]));
    }
    
    VKLOG_CORE_INFO("Image views created successfully!");
}

void VulkanContext::CreateRenderPass()
{
    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void VulkanContext::CreateFramebuffers()
{
    m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

    for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_SwapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_SwapChainExtent.width;
        framebufferInfo.height = m_SwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void VulkanContext::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow command buffers to be rerecorded individually
    
    VK_CHECK_RESULT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool));
    
    VKLOG_CORE_INFO("Command pool created successfully!");
}

void VulkanContext::CreateCommandBuffers()
{
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();
    
    if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void VulkanContext::CreateSyncObjects()
{
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start with the fence in the signaled state
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
        VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]));
        VK_CHECK_RESULT(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]));
    }
    
    VKLOG_CORE_INFO("Synchronization objects created successfully!");
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    // Map Vulkan message severity to our log levels
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            VKLOG_CORE_TRACE("Validation layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            VKLOG_CORE_INFO("Validation layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            VKLOG_CORE_WARN("Validation layer: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            VKLOG_CORE_ERROR("Validation layer: {}", pCallbackData->pMessage);
            break;
        default:
            VKLOG_CORE_INFO("Validation layer: {}", pCallbackData->pMessage);
            break;
    }
    
    return VK_FALSE; // Don't abort the function that triggered the validation layer message
}

} // namespace Valkyrion

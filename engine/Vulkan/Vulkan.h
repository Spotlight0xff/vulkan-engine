//
// Created by spotlight on 1/14/17.
//

#ifndef VULKAN_ENGINE_VULKAN_H
#define VULKAN_ENGINE_VULKAN_H

#define ENGINE_VERSION_MAJOR 0
#define ENGINE_VERSION_MINOR 1
#define ENGINE_VERSION_PATCH 0

#include "VDeleter.h"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <cstring>

namespace engine {

struct QueueFamilyIndices {
  int graphics_family = -1;
  int presentation_family = -1;

  bool isComplete() {
      return graphics_family >= 0 && presentation_family >= 0;
  }
};

// Used to query swap chain supported capabilities
struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};


class Vulkan {
  public:
    Vulkan();

    void init();
    void mainLoop();

  private:
    VDeleter<VkInstance> instance_{vkDestroyInstance};
    VDeleter<VkDebugReportCallbackEXT> debug_cb_{instance_, DestroyDebugReportCallbackEXT};
    //VDeleter<VkPhysicalDevice> device_{instance_, vkDestroyDevice};
    VDeleter<VkDevice> device_{vkDestroyDevice};
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VDeleter<VkSurfaceKHR> surface_{instance_, vkDestroySurfaceKHR};
    VkQueue graphics_queue_;
    VkQueue presentation_queue_;


    GLFWwindow* window_;
    const int width_ = 800;
    const int height_ = 600;
    const std::vector<const char*> requested_validation_layers_ = {
            "VK_LAYER_LUNARG_standard_validation"
    };

    // vector of required extensions which we check in isDeviceSuitable
    const std::vector<const char*> required_device_extensions_ = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

#ifdef NDEBUG
    const bool enable_validation_ = false;
#else
    const bool enable_validation_ = true;
#endif


    void initWindow();

    static void cbKeyboardDispatcher(
            GLFWwindow* window,
            int key, int scancode, int action, int mods);


    void cbKeyboard(
            GLFWwindow* window,
            int key, int scancode, int action, int mods);

    bool checkValidationLayers();

    // return extensions required to operate
    std::vector<const char*> getRequiredExtensions();

    void initVulkan();

    void createInstance();

    void setupDebugCallback();

    void selectPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice const& device);

    // load extension function "vkCreateDebugReportCallbackEXT"
    static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pCallback);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr) {
            func(instance, callback, pAllocator);
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugReportFlagsEXT flags,
            VkDebugReportObjectTypeEXT obj_type,
            uint64_t obj,
            size_t location,
            int32_t code,
            const char* prefix,
            const char* msg,
            void* userData);

    void createLogicalDevice();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    void createSurface();

    bool checkDeviceExtensionsSupport(VkPhysicalDevice const &device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice const &device);
};


}

#endif //VULKAN_ENGINE_VULKAN_H

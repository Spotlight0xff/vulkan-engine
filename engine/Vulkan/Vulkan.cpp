//
// Created by spotlight on 1/14/17.
//

#include <set>
#include <limits>
#include "Vulkan.h"


namespace engine {

Vulkan::Vulkan() {

}

void Vulkan::init() {
  initWindow();
  initVulkan();
}


/*
 * Initialize the Vulkan subsystem
 */
void Vulkan::initVulkan() {
  createInstance();
  setupDebugCallback();
  createSurface();
  selectPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
}

/*
 * Create vulkan instance, with validation layers if DEBUG
 */
void Vulkan::createInstance() {
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "vulkan-engine";
  app_info.applicationVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);
  app_info.pEngineName = "vulkan-engine";
  app_info.apiVersion = VK_API_VERSION_1_0;
  app_info.engineVersion = VK_MAKE_VERSION(ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH);


  auto extensions = getRequiredExtensions();

  VkInstanceCreateInfo instance_info = {};
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = extensions.size();
  instance_info.ppEnabledExtensionNames = extensions.data();
  instance_info.enabledLayerCount = 0; // global validation layers

  VkResult result = vkCreateInstance(&instance_info, nullptr, instance_.replace());
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance");
  }

  if (enable_validation_ && checkValidationLayers()) {
    instance_info.enabledLayerCount = requested_validation_layers_.size();
    instance_info.ppEnabledLayerNames = requested_validation_layers_.data();
    std::cout << "Enabled " << requested_validation_layers_.size() << " validation layers\n";
  }
  std::cout << "Initialized Vulkan instance successfully.\n";
}

/*
 * Select a suitable vulkan-capable GPU
 */
void Vulkan::selectPhysicalDevice() {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
  if (device_count == 0) {
    throw std::runtime_error("No Vulkan-capable GPUs found.");
  }

  // allocate vector of retrieved size
  std::vector<VkPhysicalDevice> devices(device_count);

  vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
  std::cout << "Number of Vulkan-capable GPUs found: " << devices.size() << "\n";

  for(uint32_t i = 0; i < devices.size(); i++) {
    if (isDeviceSuitable(devices[i])) {
      std::cout << "Found suitable physical device " << i << ".\n";
      physical_device_ = devices[i];
      break;
    }
  }

  if (physical_device_ == VK_NULL_HANDLE) {
    throw std::runtime_error("No suitable GPU found.");
  }
}

/*
 * Determine if a device has the capabilities we need
 */
bool Vulkan::isDeviceSuitable(VkPhysicalDevice const& device) {
  // check queue families
  QueueFamilyIndices indices = findQueueFamilies(device);
  if (!indices.isComplete()) {
    return false;
  }

  // check physical device extensions
  bool all_extensions_supported = checkDeviceExtensionsSupport(device);
  if (!all_extensions_supported) {
    return false;
  }

  // check swap chain support
  SwapChainSupportDetails swapchain_support = querySwapChainSupport(device);
  if (swapchain_support.formats.empty()) {
    std::cout << "no formats\n";
    return false;
  }
  if (swapchain_support.present_modes.empty()) {
    std::cout << "no presentation modes\n";
    return false;
  }

  return true;
}


/*
 * Check if the physical device implements the required device extensions
 * Returns true if all are implemented.
 */
bool Vulkan::checkDeviceExtensionsSupport(VkPhysicalDevice const& device) {
  uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available_extensions.data());

  std::set<std::string> required_extensions(required_device_extensions_.begin(),
  required_device_extensions_.end());
  for(auto const& ext : available_extensions) {
    required_extensions.erase(ext.extensionName);
  }
  return required_extensions.empty();
}


/*
 * creates the corresponding logical device from the physical one
 * needs to be called *after* selectPhysicalDevice()
 */
void Vulkan::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physical_device_);
  float queue_prio = 1.0f;
  std::set<int> unique_queue_families = {
          indices.graphics_family,
          indices.presentation_family,
  };
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  for(auto queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.queueFamilyIndex = queue_family;
    info.queueCount = 1;
    info.pQueuePriorities = &queue_prio;
    queue_create_infos.push_back(info);

  }


  // Set the used device features
  VkPhysicalDeviceFeatures features = {};


  // create logical device
  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.queueCreateInfoCount = queue_create_infos.size();
  create_info.pEnabledFeatures = &features;

  // required
  create_info.enabledExtensionCount = required_device_extensions_.size();
  create_info.ppEnabledExtensionNames = required_device_extensions_.data();

  // Set the validation layers, if are on DEBUG
  if (enable_validation_) {
    create_info.enabledLayerCount = requested_validation_layers_.size();
    create_info.ppEnabledLayerNames = requested_validation_layers_.data();
  } else {
    create_info.enabledLayerCount = 0;
  }
  if (vkCreateDevice(physical_device_, &create_info, nullptr, device_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device_, indices.graphics_family, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, indices.presentation_family, 0, &presentation_queue_);
  std::cout << "Logical device creation completed successfully.\n";
}

void Vulkan::createSurface() {
  if (glfwCreateWindowSurface(instance_, window_, nullptr, surface_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}


void Vulkan::createSwapChain() {
  SwapChainSupportDetails sc_support = querySwapChainSupport(physical_device_);

  VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(sc_support.formats);
  VkPresentModeKHR mode = chooseSwapPresentMode(sc_support.present_modes);
  VkExtent2D extent = chooseSwapExtent(sc_support.capabilities);

  // We want triple-buffering (but respect the max numbers of images)
  uint32_t image_count = sc_support.capabilities.minImageCount + 1;
  if (sc_support.capabilities.maxImageCount > 0 && image_count > sc_support.capabilities.maxImageCount) {
    image_count = sc_support.capabilities.maxImageCount;
  }

  // Now that we decided all the options, lets finally create the swap-chain!
  VkSwapchainCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = surface_;
  info.minImageCount = image_count;
  info.imageFormat = format.format;
  info.imageColorSpace = format.colorSpace;
  info.presentMode = mode;
  info.imageExtent = extent;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.imageArrayLayers = 1; // amount of layers in each image

  // we don't want any transformation for now
  info.preTransform = sc_support.capabilities.currentTransform;

  // ignore alpha channel
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  // we don't care about obscured pixels
  info.clipped = VK_TRUE;

  // we assume that we only have one swap chain ever (for now at least)
  info.oldSwapchain = VK_NULL_HANDLE;


  // Decide how we will handle swap chain images across separate queue families
  QueueFamilyIndices indices = findQueueFamilies(physical_device_);
  uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphics_family, (uint32_t) indices.presentation_family};

  // If they are on different queues, do concurrent, otherwise exclusive
  if (indices.graphics_family != indices.presentation_family) {
    info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  if (vkCreateSwapchainKHR(device_, &info, nullptr, swapchain_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  std::cout << "Created swap chain successfully.\n";
  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

  swapchain_format_ = format.format;
  swapchain_extent_ = extent;

}

QueueFamilyIndices Vulkan::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;
  uint32_t family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, queue_families.data());

  uint32_t i = 0;
  for (auto const& family : queue_families) {
    if (family.queueCount > 0) {
      // check if the queue family can do graphics
      if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphics_family = i;
      }

      // check if the queue family can present to the surface
      VkBool32 has_presentation = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &has_presentation);
      if (has_presentation) {
        indices.presentation_family = i;
      }
    }

    if (indices.isComplete()) {
      break;
    }
    i++;
  }
  return indices;

}

// popule the swapchain support structure with the capabilities
SwapChainSupportDetails Vulkan::querySwapChainSupport(VkPhysicalDevice const& device) {
  SwapChainSupportDetails details;

  // Query surface capabilities
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

  // Query available formats for surface
  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
  std::cout << "Number of surface formats found: " << format_count << "\n";

  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
  }

  // Query presentation modes for surface
  uint32_t modes_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &modes_count, nullptr);
  std::cout << "Number of presentation modes found: " << modes_count << "\n";
  if (modes_count) {
    details.present_modes.resize(modes_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &modes_count, details.present_modes.data());
  }

  return details;
}

VkSurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> const& formats) {
  // best case:
  // surface has no preferred format, we can choose freely :)
  if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    std::cout << "best case for surface\n";
    return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  }

  for (const auto& format : formats) {
    // colorspace: SRGB, format=RGB
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  // alright, just default to the first one
  return formats[0];
}

// choose the presentation mode for the surface
// just use FIFO for now (basically vsync)
VkPresentModeKHR Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> present_modes) {
  return VK_PRESENT_MODE_FIFO_KHR;
}

/*
 * choose the resolution of the swap chain images
 * If vulkan tells us to use the current extent, we do so.
 * Otherwise, get the largest resolution we can get.
 */
VkExtent2D Vulkan::chooseSwapExtent(VkSurfaceCapabilitiesKHR const& capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  VkExtent2D actual = {uint32_t(width_), uint32_t(height_)};

  actual.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual.width));
  actual.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual.height));

  return actual;
}

/*
 * Initialize GLFW window and setup input
 */
void Vulkan::initWindow() {
  glfwInit();
  // Don't create OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ = glfwCreateWindow(width_, height_, "Rendering", nullptr, nullptr);
  if (window_ == nullptr) {
    throw std::runtime_error("Failed to create GLFW window!");
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetKeyCallback(window_, cbKeyboardDispatcher);

}

/*
 * enter the main loop
 *
 * Exit through the keyboard/mouse callback
 */
void Vulkan::mainLoop() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
  }
  glfwTerminate();
}

void Vulkan::cbKeyboardDispatcher(
        GLFWwindow *window,
        int key, int scancode, int action, int mods) {

  Vulkan *app = (Vulkan *) glfwGetWindowUserPointer(window);
  if (app) {
    app->cbKeyboard(window, key, scancode, action, mods);
  }
}

/*
 * Handle keyboard callback from GLFW window
 */
void Vulkan::cbKeyboard(
        GLFWwindow *window,
        int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

bool Vulkan::checkValidationLayers() {
  uint32_t layer_count = 0;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for(auto const& requested_layer : requested_validation_layers_) {
    bool layer_found = false;
    for(auto const& available_layer : available_layers) {
      if (strcmp(requested_layer, available_layer.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      std::string error = "Could not find layer" + std::string(requested_layer);
      throw std::runtime_error(error);
    }
  }
  return true;
}

std::vector<const char*> Vulkan::getRequiredExtensions() {
  std::vector<const char*> extensions;

  unsigned int count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&count);

  for (unsigned int i = 0; i < count; i++) {
    extensions.push_back(glfw_extensions[i]);
  }

  std::cout << "We've got " << count << " Vulkan extensions by GLFW:\n";
  for(auto const& ext : extensions) {
    std::cout << "\t" << ext << "\n";
  }

  if (enable_validation_) {
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }

  return extensions;
}

void Vulkan::setupDebugCallback() {
  if (!enable_validation_) return;

  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
  createInfo.pUserData = this;
  createInfo.pfnCallback = Vulkan::debugCallback;
  if (CreateDebugReportCallbackEXT(instance_, &createInfo, nullptr, debug_cb_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug callback!");
  } else {
    std::cout << "Setup debug callback successfully.\n";
  }

}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT obj_type,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char* prefix,
        const char* msg,
        void* userData) {
  Vulkan* vulkan = (Vulkan *) userData;

  std::cerr << "[!!] Validation layer: " << prefix << ": " << msg << std::endl;

  return VK_FALSE;
}
}

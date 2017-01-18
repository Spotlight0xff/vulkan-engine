//
// Created by spotlight on 1/14/17.
//

#include <set>
#include <limits>
#include "Vulkan.h"
#include "../util.h"


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
  createImageViews();
  createRenderpass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
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


// Create the image views to the images in the swap chain
void Vulkan::createImageViews() {
  sc_image_views_.resize(swapchain_images_.size(), VDeleter<VkImageView>{device_, vkDestroyImageView});
  for(size_t i = 0; i < swapchain_images_.size(); i++) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.format = swapchain_format_;
    info.image = swapchain_images_[i];
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // default mapping:
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    if (vkCreateImageView(device_, &info, nullptr, sc_image_views_[i].replace()) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }
  }
  std::cout << "Created " << swapchain_images_.size() << " image views successfully.\n";
}


void Vulkan::createGraphicsPipeline() {
  auto vert_shader_source = util::readFile("shaders/vert.spv");
  auto frag_shader_source = util::readFile("shaders/frag.spv");


  VDeleter<VkShaderModule> vert_shader_module{device_, vkDestroyShaderModule};
  VDeleter<VkShaderModule> frag_shader_module{device_, vkDestroyShaderModule};

  createShaderModule(vert_shader_source, vert_shader_module);
  createShaderModule(frag_shader_source, frag_shader_module);

  // specify shader module in graphics pipeline
  VkPipelineShaderStageCreateInfo vert_stage_info = {};
  vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_stage_info.module = vert_shader_module;
  vert_stage_info.pName = "main";
  vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;

  // specify fragment shader in graphics pipeline
  VkPipelineShaderStageCreateInfo frag_stage_info = {};
  frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_stage_info.module = frag_shader_module;
  frag_stage_info.pName = "main";
  frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info, frag_stage_info};


  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  // We don't load vertex data for now, we specify directly in the shaders
  // We'll change this later :)
  vertex_input_info.vertexAttributeDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions = nullptr;
  vertex_input_info.vertexBindingDescriptionCount = 0;
  vertex_input_info.pVertexBindingDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.primitiveRestartEnable = VK_FALSE; // used for element buffers
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // just triangles for now


  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = float(swapchain_extent_.width);
  viewport.height = float(swapchain_extent_.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  // nothing special, draw the whole fb
  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapchain_extent_;

  // we just use one viewport and one scissor, nothing fancy here
  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;


  // perform rasterization
  VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
  rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer_info.depthClampEnable = VK_FALSE; // clamp using near & far plane
  rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
  // determine how fragments are generated for vertices:
  // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
  // VK_POLYGON_MODE_LINE: draw polygon edges as lines (requires GPU feature)
  // VK_POLYGON_MODE_POINT: polygon vertices are drawn as points (required GPU feature)
  rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer_info.lineWidth = 1.0f;
  rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT; // determine face culling
  rasterizer_info.depthBiasEnable = VK_FALSE;
  rasterizer_info.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer_info.depthBiasClamp = 0.0f; // Optional
  rasterizer_info.depthBiasSlopeFactor = 0.0f; // Optional



  // perform multi-sampling
  // We don't use it for now, but we'll change this later (TODO)
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; /// Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional


  // We don't use depth and stencil buffer r/n
  //VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};

  // perform color blending
  // this happens after the fragment shader
  // and combines it with the color from the framebuffer
  VkPipelineColorBlendAttachmentState color_blend_attach = {};
  // RGBA:
  color_blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attach.blendEnable = VK_FALSE; // disable blending for now
  color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

  VkPipelineColorBlendStateCreateInfo color_blend_info = {};
  color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend_info.logicOpEnable = VK_FALSE;
  color_blend_info.logicOp = VK_LOGIC_OP_COPY; // Optional
  color_blend_info.attachmentCount = 1;
  color_blend_info.pAttachments = &color_blend_attach;
  color_blend_info.blendConstants[0] = 0.0f; // Optional
  color_blend_info.blendConstants[1] = 0.0f; // Optional
  color_blend_info.blendConstants[2] = 0.0f; // Optional
  color_blend_info.blendConstants[3] = 0.0f; // Optional

  // with alpha-blending enabled: see VkBlendFactor and VkBlendOp
  //color_blend_attach.blendEnable = VK_TRUE;
  //color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  //color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  //color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
  //color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  //color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  //color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;

  // Set the stuff we can change without recreating the whole graphics pipeline
  VkDynamicState dynamic_states[] = {
          VK_DYNAMIC_STATE_VIEWPORT,
          VK_DYNAMIC_STATE_LINE_WIDTH
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates = dynamic_states;

  VkPipelineLayoutCreateInfo pipeline_layout_info = {};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 0; // Optional
  pipeline_layout_info.pSetLayouts = nullptr; // Optional
  pipeline_layout_info.pushConstantRangeCount = 0; // Optional
  pipeline_layout_info.pPushConstantRanges = 0; // Optional

  if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr,
                             pipeline_layout_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  // finally assemble the graphics pipeline
  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  // number of stages, we have vertex and fragment shader currently
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;

  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pRasterizationState = &rasterizer_info;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pColorBlendState = &color_blend_info;
  pipeline_info.pDepthStencilState = nullptr;
  pipeline_info.pDynamicState = nullptr;

  pipeline_info.layout = pipeline_layout_;


  // add renderpass
  pipeline_info.renderPass = renderpass_;
  pipeline_info.subpass = 0; // index of subpass

  // we derive from no pipeline
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = -1;

  // VK_NULL_HANDLE is VkPipelineCache used to speed up pipeline creation
  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, graphics_pipeline_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline");
  }



  std::cout << "Created graphics pipeline successfully.\n";
}


void Vulkan::createRenderpass() {
  // just the attachment for the swapchain image
  VkAttachmentDescription attachment = {};
  attachment.format = this->swapchain_format_;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear framebuffer
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Rendered stuff should stay in memory

  // don't care about stencil buffer
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // we would like to have the images in the swap chain

  // create the reference to the created color attachment
  VkAttachmentReference attachment_ref = {};
  attachment_ref.attachment = 0; // index
  attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // create the subpass for this renderpass
  // can be used for post-processing stuff
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // graphics subpass
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &attachment_ref;


  // finally create the render pass
  VkRenderPassCreateInfo renderpass = {};
  renderpass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpass.attachmentCount = 1;
  renderpass.pAttachments = &attachment;
  renderpass.subpassCount = 1;
  renderpass.pSubpasses = &subpass;

  if (vkCreateRenderPass(device_, &renderpass, nullptr, renderpass_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass");
  }
  std::cout << "Created render pass successfully.\n";
}

void Vulkan::createFramebuffers() {
  // match number of image views
  sc_framebuffers_.resize(sc_image_views_.size(), VDeleter<VkFramebuffer> {device_, vkDestroyFramebuffer});

  // does this work with auto const&
  for(size_t i = 0; i < sc_image_views_.size(); i++) {
    VkImageView attachments[] = {
            sc_image_views_[i]
    };

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderpass_;
    info.attachmentCount = 1;
    info.pAttachments = attachments;
    info.width = swapchain_extent_.width;
    info.height = swapchain_extent_.height;
    info.layers = 1;

    if (vkCreateFramebuffer(device_, &info, nullptr, sc_framebuffers_[i].replace()) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create framebuffer for image view");
    }
  }

  std::cout << "Number of created framebuffers: " << sc_framebuffers_.size() << "\n";
}

void Vulkan::createCommandPool() {
  QueueFamilyIndices queue_indices = findQueueFamilies(physical_device_);

  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.queueFamilyIndex = queue_indices.graphics_family;
  // possible flags:
  // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT:
  //    indicate that we rerecord the command buffers often
  info.flags = 0;

  if (vkCreateCommandPool(device_, &info, nullptr, command_pool_.replace()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool");
  }
  std::cout << "Created command pool successfully.\n";
}

void Vulkan::createCommandBuffers() {
  command_buffers_.resize(sc_framebuffers_.size());

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandBufferCount = command_buffers_.size();
  alloc_info.commandPool = command_pool_;

  // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
  // VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if (vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command bufffers");
  }

  // Begin command buffer recording
  for (size_t i=0; i < command_buffers_.size(); i++) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // we can specify the command buffer usage:
    // simultaneous: we can resubmit instantly, if we want
    // one_time: will be rerecorded after submitting
    // render_pass: used for renderpass (duh!)
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr; // we don't inherit

    if (vkBeginCommandBuffer(command_buffers_[i], &begin_info) != VK_SUCCESS) {
      throw std::runtime_error("Failed to start recording command buffer");
    }

    // now lets add the renderpass to the command buffer
    VkRenderPassBeginInfo render_info = {};
    render_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_info.renderPass = renderpass_;
    render_info.framebuffer = sc_framebuffers_[i];
    render_info.renderArea.offset = {0, 0};
    render_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_color = {0.2, 0.3, 0.3, 1.0};
    render_info.clearValueCount = 1;
    render_info.pClearValues = &clear_color;

    // we dont use a secondary command buffer (with subpass) -> inline
    vkCmdBeginRenderPass(command_buffers_[i], &render_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

    // vertex count, instance count, first vertex, first instance
    vkCmdDraw(command_buffers_[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(command_buffers_[i]);

    if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer");
    }
    std::cout << "Successfully recorded command buffer i=" << i << "\n";
  }
}

/*
 * create a new SPIR-V shadermodule from bytecode
 */
void Vulkan::createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& module) {
  VkShaderModuleCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = code.size();
  info.pCode = (uint32_t*) code.data();
  if (vkCreateShaderModule(device_, &info, nullptr, module.replace()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
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

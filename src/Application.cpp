#include "Application.hpp"

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
    const VkAllocationCallbacks *allocator,
    VkDebugUtilsMessengerEXT *debugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, createInfo, allocator, debugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *allocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, debugMessenger, allocator);
  }
}

Application::Application()
    : m_window(640, 480, "Mmmmm"), m_physicalDevice(VK_NULL_HANDLE) {
  if (!m_window.initialized()) {
    throw std::runtime_error("Failed to create window");
  }
}

Application::~Application() { cleanup(); }

void Application::init() {
  createVulkanInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapchain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
}

void Application::run() {
  while (!m_window.shouldClose()) {
    glfwPollEvents();
  }
}

void Application::createVulkanInstance() {
  if (m_enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("Validation layers requested but no available");
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hi";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_1;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto requiredExtensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = requiredExtensions.size();
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (m_enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(m_validationLayers.size());
    createInfo.ppEnabledLayerNames = m_validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledExtensionNames = nullptr;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vulkan instance");
  }

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                         extensions.data());

  std::cout << "Available extensions: " << std::endl;
  for (const auto &extension : extensions) {
    std::cout << '\t' << extension.extensionName << std::endl;
  }
}

void Application::setupDebugMessenger() {
  if (!m_enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(m_vulkanInstance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("Failed to set up debug messenger");
  }
}

void Application::createSurface() {
  m_surface = m_window.createSurface(m_vulkanInstance);
}

void Application::pickPhysicalDevice() {
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDeviceGroups(m_vulkanInstance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("No GPU with vulkan support");
  }

  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount,
                             physicalDevices.data());

  if (const auto &mostSuitable =
          mostSuitableDevice(physicalDevices, m_surface, m_deviceExtensions)) {
    physicalDevice = *mostSuitable;
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("No suitable GPU found");
  }

  m_physicalDevice = physicalDevice;
}

Application::QueueFamilyIndices
Application::findQueueFamilies(VkPhysicalDevice physicalDevice,
                               VkSurfaceKHR surface) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  for (int i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &presentSupport);

    if (presentSupport)
      indices.presentFamily = i;

    if (indices.isComplete())
      break;
  }

  return indices;
}

void Application::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

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
  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(m_deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

  if (m_enableValidationLayers) {
    deviceCreateInfo.enabledLayerCount =
        static_cast<uint32_t>(m_validationLayers.size());
    deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
  } else {
    deviceCreateInfo.enabledLayerCount = 0;
  }

  VkDevice device;
  if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device");
  }

  VkQueue graphicsQueue;
  VkQueue presentQueue;
  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

  m_device = device;
  m_graphicsQueue = graphicsQueue;
  m_presentQueue = presentQueue;
}

void Application::createSwapchain() {
  SwapchainSupportDetails swapchainSupport =
      querySwapchainSupport(m_physicalDevice, m_surface);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapchainSupport.presentModes);
  Size<int> framebufferSize = m_window.getFramebufferSize();
  VkExtent2D extent =
      chooseSwapExtent(swapchainSupport.capabilities,
                       {static_cast<uint32_t>(framebufferSize.width),
                        static_cast<uint32_t>(framebufferSize.height)});

  uint32_t imageCount =
      swapchainSupport.capabilities.minImageCount +
      ((swapchainSupport.capabilities.maxImageCount == 0 ||
        imageCount < swapchainSupport.capabilities.maxImageCount)
           ? 1
           : 0);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = m_surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_surface);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkSwapchainKHR swapchain;
  if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &swapchain) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create swapchain");
  }
  m_swapchain = swapchain;

  vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
  m_swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount,
                          m_swapchainImages.data());

  m_swapchainImageFormat = surfaceFormat.format;
  m_swapchainExtent = extent;
}

void Application::createImageViews() {
  m_swapchainImageViews.resize(m_swapchainImages.size());

  for (size_t i = 0; i < m_swapchainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_swapchainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = m_swapchainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &createInfo, nullptr,
                          &m_swapchainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image view");
    }
  }
}

void Application::createRenderPass() {
  VkAttachmentDescription colorAttachments = [this]() {
    VkAttachmentDescription desc{};
    desc.format = m_swapchainImageFormat;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return desc;
  }();

  VkAttachmentReference colorAttachmentRef = []() {
    VkAttachmentReference ref{};
    ref.attachment = 0;
    ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return ref;
  }();

  VkSubpassDescription subpass = [&colorAttachmentRef]() {
    VkSubpassDescription desc{};
    desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    desc.colorAttachmentCount = 1;
    desc.pColorAttachments = &colorAttachmentRef;
    return desc;
  }();

  VkRenderPassCreateInfo renderPassInfo = [&colorAttachments, &subpass]() {
    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &colorAttachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    return info;
  }();

  VkRenderPass renderPass;
  if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass");
  }
  m_renderPass = renderPass;
}

void Application::createGraphicsPipeline() {
  auto const readCode = [](std::string const &filename) {
    std::optional<std::vector<char>> code = Utils::readByteCode(filename);
    if (code)
      return code.value();
    else
      throw std::runtime_error("Failed to get shader code");
  };
  std::vector<char> vertShaderCode = readCode("Basic.vert.spv");
  std::vector<char> fragShaderCode = readCode("Basic.frag.spv");

  VkShaderModule vertShaderModule =
      createShaderModule(vertShaderCode, m_device);
  VkShaderModule fragShaderModule =
      createShaderModule(fragShaderCode, m_device);

  VkPipelineShaderStageCreateInfo vertStageInfo{};
  vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStageInfo.module = vertShaderModule;
  vertStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragStageInfo{};
  fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStageInfo.module = fragShaderModule;
  fragStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo,
                                                    fragStageInfo};

  VkPipelineVertexInputStateCreateInfo vertInputInfo = []() {
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = 0;
    info.pVertexBindingDescriptions = nullptr;
    info.vertexAttributeDescriptionCount = 0;
    info.pVertexAttributeDescriptions = nullptr;
    return info;
  }();

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = []() {
    VkPipelineInputAssemblyStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
  }();

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(m_swapchainExtent.width);
  viewport.height = static_cast<float>(m_swapchainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = m_swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportStateInfo = [&viewport,
                                                         &scissor]() {
    VkPipelineViewportStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = 1;
    info.pViewports = &viewport;
    info.scissorCount = 1;
    info.pScissors = &scissor;
    return info;
  }();

  VkPipelineRasterizationStateCreateInfo rasterizerInfo = []() {
    VkPipelineRasterizationStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode = VK_POLYGON_MODE_FILL;
    info.lineWidth = 1.0f;
    info.cullMode = VK_CULL_MODE_BACK_BIT;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;
    return info;
  }();

  VkPipelineMultisampleStateCreateInfo multisampling = []() {
    VkPipelineMultisampleStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.sampleShadingEnable = VK_FALSE;
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading = 1.0f;
    info.pSampleMask = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
  }();

  VkPipelineColorBlendAttachmentState colorBlendAttachment = []() {
    VkPipelineColorBlendAttachmentState state{};
    state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    state.blendEnable = VK_FALSE;
    state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    state.colorBlendOp = VK_BLEND_OP_ADD;
    state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    state.alphaBlendOp = VK_BLEND_OP_ADD;
    return state;
  }();

  VkPipelineColorBlendStateCreateInfo colorBlendState =
      [&colorBlendAttachment]() {
        VkPipelineColorBlendStateCreateInfo state{};
        state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        state.logicOpEnable = VK_FALSE;
        state.logicOp = VK_LOGIC_OP_COPY;
        state.attachmentCount = 1;
        state.pAttachments = &colorBlendAttachment;
        state.blendConstants[0] = 0.0f;
        state.blendConstants[1] = 0.0f;
        state.blendConstants[2] = 0.0f;
        state.blendConstants[3] = 0.0f;
        return state;
      }();

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_LINE_WIDTH};

  VkPipelineDynamicStateCreateInfo dynamicState = [&dynamicStates]() {
    VkPipelineDynamicStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    info.pDynamicStates = dynamicStates.data();
    return info;
  }();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = []() {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
  }();

  VkPipelineLayout pipelineLayout;
  if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout");
  }
  m_pipelineLayout = pipelineLayout;

  VkGraphicsPipelineCreateInfo pipelineInfo =
      [&shaderStages, &vertInputInfo, &inputAssemblyInfo, &viewportStateInfo,
       &rasterizerInfo, &multisampling, &colorBlendState, this]() {
        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.stageCount = 2;
        info.pStages = shaderStages;
        info.pVertexInputState = &vertInputInfo;
        info.pInputAssemblyState = &inputAssemblyInfo;
        info.pViewportState = &viewportStateInfo;
        info.pRasterizationState = &rasterizerInfo;
        info.pMultisampleState = &multisampling;
        info.pDepthStencilState = nullptr;
        info.pColorBlendState = &colorBlendState;
        info.pDynamicState = nullptr;
        info.layout = m_pipelineLayout;
        info.renderPass = m_renderPass;
        info.subpass = 0;
        info.basePipelineHandle = VK_NULL_HANDLE;
        info.basePipelineIndex = -1;
        return info;
      }();

  VkPipeline graphicsPipeline;
  if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline");
  }
  m_graphicsPipeline = graphicsPipeline;

  vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
  vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void Application::cleanup() {
  vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
  vkDestroyRenderPass(m_device, m_renderPass, nullptr);

  for (VkImageView imageView : m_swapchainImageViews) {
    vkDestroyImageView(m_device, imageView, nullptr);
  }

  vkDestroyDevice(m_device, nullptr);
  if (m_enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(m_vulkanInstance, debugMessenger, nullptr);
  }
  vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
  vkDestroyInstance(m_vulkanInstance, nullptr);
}

bool Application::checkValidationLayerSupport() const {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : m_validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
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

std::vector<const char *> Application::getRequiredExtensions() {
  uint32_t extensionsCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + extensionsCount);

  if (m_enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

std::optional<VkPhysicalDevice> Application::mostSuitableDevice(
    std::vector<VkPhysicalDevice> const &physicalDevices, VkSurfaceKHR surface,
    std::vector<char const *> const &requiredExtensions) {
  auto const score = [&surface, &requiredExtensions](
                         VkPhysicalDevice device) -> std::optional<int> {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported =
        checkDeviceExtensionSupport(device, requiredExtensions);
    bool swapchainAdequate = extensionsSupported ? [&device, &surface](){
      SwapchainSupportDetails details = querySwapchainSupport(device, surface);
      return !details.formats.empty() && !details.presentModes.empty();
    }() : false;

    if (!indices.isComplete() || !extensionsSupported || !swapchainAdequate)
      return std::nullopt;

    int score =
        properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1 : 0;
    return score;
  };

  std::set<std::pair<VkPhysicalDevice, int>,
           std::function<bool(std::pair<VkPhysicalDevice, int> const &,
                              std::pair<VkPhysicalDevice, int> const &)>>
      deviceList(
          [](auto const &a, auto const &b) { return a.second < b.second; });

  for (const auto &device : physicalDevices) {
    std::optional<int> deviceScore = score(device);
    if (deviceScore) {
      deviceList.insert(std::pair(device, deviceScore.value()));
    }
  }

  return deviceList.size() > 0 ? std::make_optional(deviceList.begin()->first)
                               : std::nullopt;
}

bool Application::checkDeviceExtensionSupport(
    VkPhysicalDevice physicalDevice,
    std::vector<char const *> const &requiredExtensions) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::unordered_set<std::string> requiredExtensionSet(
      requiredExtensions.begin(), requiredExtensions.end());

  for (auto const &extension : availableExtensions) {
    requiredExtensionSet.erase(extension.extensionName);
  }

  return requiredExtensionSet.empty();
}

Application::SwapchainSupportDetails
Application::querySwapchainSupport(VkPhysicalDevice physicalDevice,
                                   VkSurfaceKHR surface) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                              &presentModeCount,
                                              details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(
    std::vector<VkSurfaceFormatKHR> const &formats) {
  for (auto const &format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return format;
  }

  return formats[0];
}

VkPresentModeKHR
Application::chooseSwapPresentMode(std::vector<VkPresentModeKHR> const &modes) {
  for (auto const &mode : modes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return mode;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
Application::chooseSwapExtent(VkSurfaceCapabilitiesKHR const &capabilities,
                              Size<uint32_t> windowSize) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {windowSize.width, windowSize.height};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

VkShaderModule Application::createShaderModule(std::vector<char> const &code,
                                               VkDevice device) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<uint32_t const *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module");
  }

  return shaderModule;
}

void Application::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData) {
  std::cerr << "Validation layer: " << callbackData->pMessage << std::endl;
  return VK_FALSE;
}
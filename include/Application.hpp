#pragma once

#include "MainWindow.hpp"
#include "Utils.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
    const VkAllocationCallbacks *allocator,
    VkDebugUtilsMessengerEXT *debugMessenger);

static void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks *allocator);

class Application {
public:
  Application();
  ~Application();

  void init();
  void run();

private:
  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }
  };

  struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  Window::MainWindow m_window;

  VkInstance m_vulkanInstance;
  VkSurfaceKHR m_surface;
  VkPhysicalDevice m_physicalDevice;
  VkDevice m_device;
  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;
  VkSwapchainKHR m_swapchain;
  std::vector<VkImage> m_swapchainImages;
  VkFormat m_swapchainImageFormat;
  VkExtent2D m_swapchainExtent;
  std::vector<VkImageView> m_swapchainImageViews;
  VkRenderPass m_renderPass;
  VkPipelineLayout m_pipelineLayout;
  VkPipeline m_graphicsPipeline;
  std::vector<VkFramebuffer> m_swapchainFramebuffers;
  VkCommandPool m_commandPool;
  VkCommandBuffer m_commandBuffer;
  VkSemaphore m_imageAvailableSemaphore;
  VkSemaphore m_renderFinishedSemaphore;
  VkFence m_inFlightFence;

  VkDebugUtilsMessengerEXT debugMessenger;

  const std::vector<char const *> m_validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  const std::vector<char const *> m_deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
  const bool m_enableValidationLayers = false;
#else
  const bool m_enableValidationLayers = true;
#endif

  void createVulkanInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapchain();
  void createImageViews();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffer();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t index);
  void drawFrame();
  void createSyncObjects();
  void cleanup();

  bool checkValidationLayerSupport() const;
  std::vector<const char *> getRequiredExtensions();

  static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface);
  static std::optional<VkPhysicalDevice>
  mostSuitableDevice(const std::vector<VkPhysicalDevice> &physicalDevices,
                     VkSurfaceKHR surface,
                     std::vector<char const *> const &requiredExtensions);
  static bool checkDeviceExtensionSupport(
      VkPhysicalDevice physicalDevice,
      std::vector<char const *> const &requiredExtensions);
  static SwapchainSupportDetails
  querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
  static VkSurfaceFormatKHR
  chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> const &formats);
  static VkPresentModeKHR
  chooseSwapPresentMode(std::vector<VkPresentModeKHR> const &modes);
  static VkExtent2D
  chooseSwapExtent(VkSurfaceCapabilitiesKHR const &capabilities,
                   Size<uint32_t> windowSize);
  static VkShaderModule createShaderModule(std::vector<char> const &code,
                                           VkDevice device);

  static void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData);
};
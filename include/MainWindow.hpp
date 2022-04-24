#pragma once

#include "Size.hpp"
#include "WindowInterface.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>

namespace Window {

class MainWindow : public WindowInterface {
public:
  MainWindow(int width, int height, const std::string &title);
  ~MainWindow();

  bool initialized() const;
  void setTitle(const std::string &title);
  Size<int> getSize() const;
  Size<int> getFramebufferSize() const;
  void setSize(Size<int> size);
  bool shouldClose() const;
  void swapBuffers();

  VkSurfaceKHR createSurface(VkInstance instance) const;

private:
  struct WindowDeleter {
    void operator()(GLFWwindow *window) { glfwDestroyWindow(window); }
  };

  std::unique_ptr<GLFWwindow, WindowDeleter> m_window;
};

} // namespace Window
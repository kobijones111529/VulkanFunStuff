#include "MainWindow.hpp"

namespace Window {

MainWindow::MainWindow(int width, int height, const std::string &title) {
  GLFWwindow *window =
      glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  m_window = std::unique_ptr<GLFWwindow, WindowDeleter>(window);
}

MainWindow::~MainWindow() {}

bool MainWindow::initialized() const { return (bool)m_window; }

void MainWindow::setTitle(const std::string &title) {
  glfwSetWindowTitle(m_window.get(), title.c_str());
}

Size<int> MainWindow::getSize() const {
  Size<int> size;
  glfwGetWindowSize(m_window.get(), &size.width, &size.height);
  return size;
}

Size<int> MainWindow::getFramebufferSize() const {
  Size<int> size;
  glfwGetFramebufferSize(m_window.get(), &size.width, &size.height);
  return size;
}

void MainWindow::setSize(Size<int> size) {
  glfwSetWindowSize(m_window.get(), size.width, size.height);
}

bool MainWindow::shouldClose() const {
  return glfwWindowShouldClose(m_window.get());
}

void MainWindow::swapBuffers() { glfwSwapBuffers(m_window.get()); }

VkSurfaceKHR MainWindow::createSurface(VkInstance instance) const {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance, m_window.get(), nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface");
  }
  return surface;
}

} // namespace Window
#include "Application.hpp"
#include "MainWindow.hpp"

#include <GLFW/glfw3.h>

#include <iostream>

int main(int, char **) {
  const auto glfwErrorCallback = [](int error, const char *description) {
    std::cerr << "GLFW error: " << description << std::endl;
  };

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    exit(EXIT_FAILURE);
  }

  glfwSetErrorCallback(glfwErrorCallback);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  Application app;
  app.init();
  app.run();

  glfwTerminate();
}

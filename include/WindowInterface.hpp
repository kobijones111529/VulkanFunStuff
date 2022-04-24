#pragma once

#include "Size.hpp"

#include <string>

namespace Window {

class WindowInterface {
public:
  virtual void setTitle(const std::string &title) = 0;
  virtual Size<int> getSize() const = 0;
  virtual Size<int> getFramebufferSize() const = 0;
  virtual void setSize(Size<int> size) = 0;
  virtual bool shouldClose() const = 0;
};

}
#include "Application.h"
#include <iostream>

#include <vector>

int main() {
  {
    Application app{};
    app.run();
  }
  VulkanApplicationContext::destroyInstance();
  return 0;
}
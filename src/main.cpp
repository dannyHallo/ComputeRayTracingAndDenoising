#include "Application.h"
#include <iostream>

#include <vector>

int main() {
  {
    Application app{};
    app.run();
  }
  VulkanApplicationContext::destroyInstance(); // TODO:

  return 0;
}
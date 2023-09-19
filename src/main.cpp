#include "Application.h"
#include <iostream>

#include <vector>

int main() {
  {
    Application app{};
    app.run();
  }
  return 0;
  // after returning from main , the destructor of vulkanApplicationContext will be called
}
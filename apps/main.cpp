#include "application/Application.hpp"
#include "utils/logger/Logger.hpp"

int main() {
  Logger logger{};
  Application app{&logger};
  app.run();
  return 0;
}

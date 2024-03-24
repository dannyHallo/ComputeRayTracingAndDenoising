#include "spdlog/spdlog.h"

#include <iostream>

int main() {
  auto logger = spdlog::default_logger();
  logger->set_level(spdlog::level::debug);
  logger->debug("Hello, {}!", "World!");
  return 0;
}

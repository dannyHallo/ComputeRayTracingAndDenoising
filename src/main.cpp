#include "Application.h"
#include <iostream>

int main() {
  // create an instance of the Application class
  Application app{};

  try {
    // run the application
    app.run();
  } catch (const std::exception &e) {
    // catch any exceptions thrown by the Application class and print the error message to the console
    std::cerr << e.what() << std::endl;
    // return EXIT_FAILURE to indicate that the program has failed
    return EXIT_FAILURE;
  }

  // return EXIT_SUCCESS to indicate that the program has run successfully
  return EXIT_SUCCESS;
}
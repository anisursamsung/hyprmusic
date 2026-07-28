#include "Core/HyprMusicApp.hpp"
#include <iostream>
#include <exception>

int main() {
  try {
    Core::HyprMusicApp app;
    app.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
}
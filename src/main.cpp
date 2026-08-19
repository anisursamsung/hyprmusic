#include "Core/HyprMusicApp.hpp"
#include <iostream>
#include <exception>
#include <vector>
#include <string>

int main(int argc, char *argv[]) {
  try {
    std::vector<std::string> initialFiles;
    for (int i = 1; i < argc; ++i) {
      if (argv[i] && argv[i][0] != '\0') {
        initialFiles.push_back(argv[i]);
      }
    }

    Core::HyprMusicApp app(initialFiles);
    app.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
}
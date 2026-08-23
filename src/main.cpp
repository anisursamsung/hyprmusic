#include "Core/HlMusicApp.hpp"
#include <iostream>
#include <exception>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

static std::string ipcSocketPath() {
  return "/tmp/hlmusic-" + std::to_string(::getuid()) + ".sock";
}

// Returns true if a running instance was found and the args were forwarded.
static bool trySendToExistingInstance(int argc, char *argv[]) {
  std::string path = ipcSocketPath();

  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    return false;

  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  ::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

  if (::connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    ::close(fd);
    return false;
  }

  // Connected to running instance.
  // Send file args separated by newlines, terminated by an empty line.
  std::string msg;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] && argv[i][0] != '\0') {
      msg += argv[i];
      msg += '\n';
    }
  }
  msg += '\n'; // empty line = end of message

  ::send(fd, msg.c_str(), msg.size(), 0);
  ::close(fd);
  return true;
}

int main(int argc, char *argv[]) {
  // If another hlmusic is already running, forward any file args to it and exit.
  if (trySendToExistingInstance(argc, argv)) {
    return 0;
  }

  try {
    Core::HlMusicApp app(argc, argv);
    app.run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
}
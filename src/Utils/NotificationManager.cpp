#include "NotificationManager.hpp"
#include <cstdlib>
#include <string>

namespace Utils {

void NotificationManager::showNotification(const std::string &msg) {
  if (msg.empty())
    return;

  // Escape single quotes for safe shell command execution
  std::string escapedMsg;
  for (char c : msg) {
    if (c == '\'') {
      escapedMsg += "'\"'\"'";
    } else {
      escapedMsg += c;
    }
  }

  std::string cmd = "notify-send -a \"hyprMusic\" -i \"multimedia-audio-player\" \"hyprMusic\" '" + escapedMsg + "' &";
  ::system(cmd.c_str());
}

} // namespace Utils

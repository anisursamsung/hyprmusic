#pragma once
#include <string>
#include <cstdio>

namespace Utils {

inline std::string formatTime(unsigned int seconds) {
  unsigned int mins = seconds / 60;
  unsigned int secs = seconds % 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%u:%02u", mins, secs);
  return std::string(buf);
}

inline std::string sanitizePlaylistName(const std::string &name) {
  std::string clean = name;
  clean.erase(0, clean.find_first_not_of(" \t\n\r"));
  clean.erase(clean.find_last_not_of(" \t\n\r") + 1);
  return clean;
}

} // namespace Utils

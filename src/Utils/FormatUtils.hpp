#pragma once
#include <string>
#include <cstdio>
#include <sstream>

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

inline std::string truncateText(const std::string &str, size_t maxLen = 65) {
  if (str.length() <= maxLen)
    return str;
  return str.substr(0, maxLen - 3) + "...";
}

inline std::string wrapText(const std::string &str, size_t maxCharsPerLine = 24) {
  if (str.empty() || str.length() <= maxCharsPerLine)
    return str;
  std::string result;
  size_t currentLineLength = 0;
  std::string word;
  std::istringstream stream(str);
  while (stream >> word) {
    if (currentLineLength + word.length() > maxCharsPerLine && currentLineLength > 0) {
      result += "\n";
      currentLineLength = 0;
    } else if (currentLineLength > 0) {
      result += " ";
      currentLineLength += 1;
    }
    result += word;
    currentLineLength += word.length();
  }
  return result;
}

} // namespace Utils

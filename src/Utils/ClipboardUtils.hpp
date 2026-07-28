#pragma once
#include <cstdio>
#include <string>

namespace Utils {

inline bool copyToClipboard(const std::string &text) {
  if (text.empty())
    return false;

  // Try wl-copy first for Wayland, then xclip for X11, then xsel fallback
  FILE *pipe = popen("wl-copy 2>/dev/null || xclip -selection clipboard 2>/dev/null || xsel --clipboard --input 2>/dev/null", "w");
  if (pipe) {
    fwrite(text.c_str(), 1, text.length(), pipe);
    pclose(pipe);
    return true;
  }
  return false;
}

inline std::string readFromClipboard() {
  FILE *pipe = popen("wl-paste --no-newline 2>/dev/null || xclip -selection clipboard -o 2>/dev/null || xsel --clipboard --output 2>/dev/null", "r");
  if (!pipe)
    return "";

  char buffer[4096];
  std::string result = "";
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);
  return result;
}

} // namespace Utils

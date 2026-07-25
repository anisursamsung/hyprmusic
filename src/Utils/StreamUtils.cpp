#include "StreamUtils.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

std::string getUserHomeDir() {
  const char *homeEnv = getenv("HOME");
  if (homeEnv && strlen(homeEnv) > 0) {
    return std::string(homeEnv);
  }
  struct passwd *pw = getpwuid(getuid());
  if (pw && pw->pw_dir) {
    return std::string(pw->pw_dir);
  }
  return "";
}

std::string escapeShellArg(const std::string &arg) {
  std::string escaped;
  for (char c : arg) {
    if (c == '"' || c == '\\' || c == '`' || c == '$') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}

std::string getJsonStringField(const std::string &json,
                                      const std::string &key) {
  std::string pattern = "\"" + key + "\": \"";
  size_t pos = json.find(pattern);
  if (pos == std::string::npos) {
    pattern = "\"" + key + "\":\"";
    pos = json.find(pattern);
    if (pos == std::string::npos)
      return "";
  }
  pos += pattern.length();
  std::string val;
  bool escaped = false;
  for (size_t i = pos; i < json.length(); ++i) {
    char c = json[i];
    if (escaped) {
      if (c == 'n')
        val += '\n';
      else if (c == 't')
        val += '\t';
      else
        val += c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      break;
    } else {
      val += c;
    }
  }
  return val;
}

std::string getJsonDuration(const std::string &json) {
  std::string durStr = getJsonStringField(json, "duration_string");
  if (!durStr.empty())
    return durStr;

  if (json.find("\"is_live\": true") != std::string::npos ||
      json.find("\"is_live\":true") != std::string::npos) {
    return "LIVE";
  }

  std::string pattern = "\"duration\": ";
  size_t pos = json.find(pattern);
  if (pos == std::string::npos) {
    pattern = "\"duration\":";
    pos = json.find(pattern);
  }
  if (pos != std::string::npos) {
    pos += pattern.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t'))
      pos++;
    std::string num;
    while (pos < json.length() &&
           (std::isdigit(json[pos]) || json[pos] == '.')) {
      num += json[pos++];
    }
    if (!num.empty()) {
      try {
        int seconds = std::stoi(num);
        int mins = seconds / 60;
        int secs = seconds % 60;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d", mins, secs);
        return std::string(buf);
      } catch (...) {
      }
    }
  }
  return "";
}

std::string extractDirectStreamUrl(const std::string &inputUrl) {
  if (inputUrl.find("youtube.com") != std::string::npos ||
      inputUrl.find("youtu.be") != std::string::npos ||
      inputUrl.find("ytsearch") != std::string::npos) {
    std::string cmd = "yt-dlp -f \"ba/b\" -g \"" + escapeShellArg(inputUrl) +
                      "\" 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (pipe) {
      char buf[8192];
      std::string streamUrl;
      if (fgets(buf, sizeof(buf), pipe) != nullptr) {
        streamUrl = buf;
        while (!streamUrl.empty() &&
               (streamUrl.back() == '\n' || streamUrl.back() == '\r')) {
          streamUrl.pop_back();
        }
      }
      pclose(pipe);
      if (!streamUrl.empty())
        return streamUrl;
    }
  }
  return inputUrl;
}

#pragma once
#include <string>
#include <unordered_map>

namespace Services {

class SettingsManager {
public:
  static std::string getMpdConfPath();
  static std::unordered_map<std::string, std::string> parseMpdConfig(const std::string &path);
  static void saveMpdConfig(const std::string &path, const std::unordered_map<std::string, std::string> &settings);
};

} // namespace Services

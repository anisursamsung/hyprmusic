#pragma once
#include <string>

std::string getUserHomeDir();
std::string escapeShellArg(const std::string &arg);
std::string getJsonStringField(const std::string &json, const std::string &key);
std::string getJsonDuration(const std::string &json);
std::string extractDirectStreamUrl(const std::string &inputUrl);

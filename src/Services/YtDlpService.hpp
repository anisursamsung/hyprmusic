#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>

namespace Services {

struct YtDlpResult {
  std::string id;
  std::string title;
  std::string uploader;
  std::string duration;
  std::string url;
};

class YtDlpService {
public:
  void setUrlTitle(const std::string &url, const std::string &title, const std::string &uploader = "");
  bool getUrlTitle(const std::string &url, std::string &title, std::string &uploader);

  void triggerSearch(const std::string &query, int count, 
                     std::function<void(const std::vector<YtDlpResult>&, bool isPlaylist, const std::string& plTitle, const std::string& plId)> callback);

private:
  std::unordered_map<std::string, std::pair<std::string, std::string>> m_urlTitleMap;
  std::mutex m_titleMapMutex;
};

} // namespace Services

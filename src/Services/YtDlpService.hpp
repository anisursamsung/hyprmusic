#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>

#include <hyprtoolkit/core/Backend.hpp>
#include <mpd/client.h>

namespace Services {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

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

  void downloadTrack(
      const std::string &url, const std::string &title, const std::string &destDir,
      CSharedPointer<IBackend> backend,
      std::function<void(const std::string &msg)> showNotification,
      std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand);

private:
  std::unordered_map<std::string, std::pair<std::string, std::string>> m_urlTitleMap;
  std::mutex m_titleMapMutex;
};

} // namespace Services

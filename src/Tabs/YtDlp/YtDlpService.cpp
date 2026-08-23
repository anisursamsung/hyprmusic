#include "YtDlpService.hpp"
#include "Utils/StreamUtils.hpp"
#include <cstdio>
#include <thread>

namespace Services {

void YtDlpService::setUrlTitle(const std::string &url, const std::string &title,
                                const std::string &uploader) {
  std::lock_guard<std::mutex> lock(m_titleMapMutex);
  m_urlTitleMap[url] = {title, uploader};
}

bool YtDlpService::getUrlTitle(const std::string &url, std::string &title,
                                std::string &uploader) {
  std::lock_guard<std::mutex> lock(m_titleMapMutex);
  auto it = m_urlTitleMap.find(url);
  if (it != m_urlTitleMap.end()) {
    title = it->second.first;
    uploader = it->second.second;
    return true;
  }
  return false;
}

void YtDlpService::triggerSearch(
    const std::string &query, int count,
    std::function<void(const std::vector<YtDlpResult> &, bool isPlaylist,
                       const std::string &plTitle, const std::string &plId)>
        callback) {
  if (query.empty())
    return;

  if (count < 1)
    count = 1;
  if (count > 50)
    count = 50;

  bool isPlaylistUrl = (query.find("list=") != std::string::npos ||
                        query.find("playlist") != std::string::npos);

  std::thread([query, count, isPlaylistUrl, callback]() {
    std::string escapedTitle = escapeShellArg(query);
    std::string cmd;
    if (isPlaylistUrl) {
      cmd = "yt-dlp --flat-playlist -j " + escapedTitle + " 2>/dev/null";
    } else {
      cmd = "yt-dlp --flat-playlist -j \"ytsearch" + std::to_string(count) +
            ":" + escapedTitle + "\" 2>/dev/null";
    }

    FILE *pipe = popen(cmd.c_str(), "r");
    std::vector<YtDlpResult> results;
    std::string detectedPlTitle = "";
    std::string detectedPlId = "";

    if (pipe) {
      char buffer[8192];
      while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        YtDlpResult res;
        res.title = getJsonStringField(line, "title");
        res.uploader = getJsonStringField(line, "uploader");
        if (res.uploader.empty())
          res.uploader = getJsonStringField(line, "channel");
        res.url = getJsonStringField(line, "url");
        if (res.url.empty())
          res.url = getJsonStringField(line, "webpage_url");
        res.id = getJsonStringField(line, "id");
        if (res.url.empty() && !res.id.empty()) {
          res.url = "https://www.youtube.com/watch?v=" + res.id;
        }
        res.duration = getJsonDuration(line);
        if (!res.title.empty() && !res.url.empty()) {
          results.push_back(res);
        }

        if (isPlaylistUrl && detectedPlTitle.empty()) {
          detectedPlTitle = getJsonStringField(line, "playlist_title");
          detectedPlId = getJsonStringField(line, "playlist_id");
        }
      }
      pclose(pipe);
    }

    std::string plTitle = detectedPlTitle.empty() ? "Imported YouTube Playlist"
                                                  : detectedPlTitle;
    callback(results, isPlaylistUrl, plTitle, detectedPlId);
  }).detach();
}

} // namespace Services

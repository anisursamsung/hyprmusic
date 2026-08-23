#pragma once
#include <mpd/client.h>
#include <functional>
#include <string>
#include <unordered_map>

namespace Services {

class MPDManager {
public:
  static void ensureMpdRunningAndConfigured(std::unordered_map<std::string, std::string> &settings);
  static void runMpdCommand(const std::function<void(struct mpd_connection *)> &cmd);

  static void togglePlayPause(std::function<void()> onUpdateStatus);
  static void prevTrack(std::function<void()> onUpdateStatus);
  static void nextTrack(std::function<void()> onUpdateStatus);
  static void playSongId(int songId, std::function<void()> onUpdateStatus);

  static void addSongToQueue(const std::string &uri, std::function<void(const std::string&)> showNotification, std::function<void()> onUpdateStatus);
  static void playSongFromUri(const std::string &uri, std::function<void(const std::string&)> showNotification, std::function<void()> onUpdateStatus);
  static void removeSongFromQueue(int songId, std::function<void()> onUpdateStatus);
};

} // namespace Services

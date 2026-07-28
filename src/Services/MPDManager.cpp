#include "MPDManager.hpp"
#include "SettingsManager.hpp"
#include "../Utils/StreamUtils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>

namespace Services {

void MPDManager::ensureMpdRunningAndConfigured(
    std::unordered_map<std::string, std::string> &settings) {
  struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);
  bool isConnected =
      conn && (mpd_connection_get_error(conn) == MPD_ERROR_SUCCESS);
  if (conn) {
    mpd_connection_free(conn);
  }

  if (isConnected) {
    settings = SettingsManager::parseMpdConfig(SettingsManager::getMpdConfPath());
    return;
  }

  std::string confPath = SettingsManager::getMpdConfPath();
  std::filesystem::path p(confPath);
  if (!std::filesystem::exists(p)) {
    std::filesystem::create_directories(p.parent_path());

    std::string homeDir = getUserHomeDir();

    std::filesystem::create_directories(homeDir + "/.config/mpd/playlists");
    std::filesystem::create_directories(homeDir + "/Music");

    std::ofstream file(confPath);
    if (file.is_open()) {
      file << "# Files and directories\n"
           << "music_directory     \"" << homeDir << "/Music\"\n"
           << "playlist_directory  \"" << homeDir
           << "/.config/mpd/playlists\"\n"
           << "db_file             \"" << homeDir
           << "/.config/mpd/database\"\n"
           << "log_file            \"" << homeDir << "/.config/mpd/log\"\n"
           << "pid_file            \"" << homeDir << "/.config/mpd/pid\"\n"
           << "state_file          \"" << homeDir << "/.config/mpd/state\"\n"
           << "sticker_file        \"" << homeDir
           << "/.config/mpd/sticker.sql\"\n\n"
           << "# Network\n"
           << "bind_to_address     \"127.0.0.1\"\n"
           << "port                \"6600\"\n"
           << "restore_paused      \"yes\"\n"
           << "auto_update         \"yes\"\n\n"
           << "# Audio Output for PipeWire (via Pulse plugin)\n"
           << "audio_output {\n"
           << "        type            \"pulse\"\n"
           << "        name            \"PipeWire Sound Server\"\n"
           << "}\n\n"
           << "# Optional: Visualizer output\n"
           << "audio_output {\n"
           << "    type                    \"fifo\"\n"
           << "    name                    \"my_fifo\"\n"
           << "    path                    \"/tmp/mpd.fifo\"\n"
           << "    format                  \"44100:16:2\"\n"
           << "}\n";
      file.close();
    }
  }

  settings = SettingsManager::parseMpdConfig(confPath);

  std::string startCmd = "mpd " + confPath + " >/dev/null 2>&1";
  int ret = std::system(startCmd.c_str());
  (void)ret;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void MPDManager::runMpdCommand(
    const std::function<void(struct mpd_connection *)> &cmd) {
  struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);
  if (!conn) {
    std::cerr << "MPD: Failed to create connection" << std::endl;
    return;
  }

  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    std::cerr << "MPD Connection Error: "
              << mpd_connection_get_error_message(conn) << std::endl;
    mpd_connection_free(conn);
    return;
  }

  cmd(conn);
  mpd_connection_free(conn);
}

void MPDManager::togglePlayPause(std::function<void()> onUpdateStatus) {
  runMpdCommand([](struct mpd_connection *conn) {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
      enum mpd_state state = mpd_status_get_state(status);
      bool pause = (state == MPD_STATE_PLAY);
      mpd_run_pause(conn, pause);
      mpd_status_free(status);
      std::cout << "MPD: Toggled pause state." << std::endl;
    }
  });
  if (onUpdateStatus)
    onUpdateStatus();
}

void MPDManager::prevTrack(std::function<void()> onUpdateStatus) {
  runMpdCommand([](struct mpd_connection *conn) {
    mpd_run_previous(conn);
    std::cout << "MPD: Prev track." << std::endl;
  });
  if (onUpdateStatus)
    onUpdateStatus();
}

void MPDManager::nextTrack(std::function<void()> onUpdateStatus) {
  runMpdCommand([](struct mpd_connection *conn) {
    mpd_run_next(conn);
    std::cout << "MPD: Next track." << std::endl;
  });
  if (onUpdateStatus)
    onUpdateStatus();
}

void MPDManager::playSongId(int songId, std::function<void()> onUpdateStatus) {
  runMpdCommand([songId](struct mpd_connection *conn) {
    mpd_run_play_id(conn, songId);
    std::cout << "MPD: Play song ID " << songId << std::endl;
  });
  if (onUpdateStatus)
    onUpdateStatus();
}

void MPDManager::addSongToQueue(
    const std::string &uri,
    std::function<void(const std::string &)> showNotification,
    std::function<void()> onUpdateStatus) {
  if (uri.empty())
    return;

  runMpdCommand([uri, showNotification](struct mpd_connection *conn) {
    bool alreadyInQueue = false;
    if (conn && mpd_send_list_queue_meta(conn)) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        const char *qUri = mpd_song_get_uri(s);
        if (qUri && std::string(qUri) == uri) {
          alreadyInQueue = true;
        }
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }

    if (alreadyInQueue) {
      std::cout << "MPD: Song already in queue: " << uri << std::endl;
      if (showNotification)
        showNotification("Already in queue");
    } else {
      if (mpd_run_add(conn, uri.c_str())) {
        std::cout << "MPD: Added song to queue: " << uri << std::endl;
        if (showNotification)
          showNotification("Added");
      } else {
        std::cerr << "MPD: Failed to add song: " << uri << std::endl;
      }
    }
  });
  if (onUpdateStatus)
    onUpdateStatus();
}

void MPDManager::playSongFromUri(
    const std::string &uri,
    std::function<void(const std::string &)> showNotification,
    std::function<void()> onUpdateStatus) {
  if (uri.empty())
    return;

  runMpdCommand([uri, showNotification](struct mpd_connection *conn) {
    int existingId = -1;
    if (conn && mpd_send_list_queue_meta(conn)) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        const char *qUri = mpd_song_get_uri(s);
        if (qUri && std::string(qUri) == uri) {
          existingId = mpd_song_get_id(s);
        }
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }

    if (existingId >= 0) {
      mpd_run_play_id(conn, existingId);
      if (showNotification)
        showNotification("Item already in queue, playing anyway");
    } else {
      int songId = mpd_run_add_id(conn, uri.c_str());
      if (songId >= 0) {
        mpd_run_play_id(conn, songId);
      }
    }
  });

  if (onUpdateStatus)
    onUpdateStatus();
}

void MPDManager::removeSongFromQueue(int songId,
                                      std::function<void()> onUpdateStatus) {
  runMpdCommand([songId](struct mpd_connection *conn) {
    if (mpd_run_delete_id(conn, songId)) {
      std::cout << "MPD: Removed song ID " << songId << " from queue"
                << std::endl;
    } else {
      std::cerr << "MPD: Failed to remove song ID " << songId << " from queue"
                << std::endl;
    }
  });
  if (onUpdateStatus)
    onUpdateStatus();
}

} // namespace Services

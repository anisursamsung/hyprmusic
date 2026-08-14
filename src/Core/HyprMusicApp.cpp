#include "HyprMusicApp.hpp"
#include "../UI/Dialogs/AddItemDialog.hpp"
#include "../UI/Dialogs/CreatePlaylistDialog.hpp"
#include "../UI/Dialogs/PlaylistSelectionDialog.hpp"
#include "../UI/Dialogs/RenamePlaylistDialog.hpp"
#include "../Utils/StreamUtils.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace Core {

HyprMusicApp::HyprMusicApp() {
  m_backend = IBackend::create();
  if (!m_backend) {
    throw std::runtime_error("Failed to create backend");
  }
}

void HyprMusicApp::run() {
  Services::MPDManager::ensureMpdRunningAndConfigured(m_mpdSettings);
  createWindow();
  createUI();
  setupEventHandlers();
  updateStatus();
  setupTimer();
  std::cout << "Starting HyprMusic..." << std::endl;
  m_window->open();
  m_backend->enterLoop();
}

void HyprMusicApp::createWindow() {
  m_window = CWindowBuilder::begin()
                 ->type(HT_WINDOW_TOPLEVEL)
                 ->appTitle("HyprMusic")
                 ->appClass("hyprmusic")
                 ->preferredSize({0, 0})
                 ->minSize({600, 400})
                 ->commence();
  if (!m_window) {
    throw std::runtime_error("Failed to create window");
  }
}

void HyprMusicApp::createUI() {
  m_palette = CPalette::palette();
  m_fontFamily =
      m_palette ? std::string(m_palette->m_vars.fontFamily) : "Sans Serif";

  auto palette = m_palette;
  std::string fontFamily = m_fontFamily;

  auto root =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  m_window->m_rootElement = root;

  auto mainBg =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.1, 0.1, 0.1, 1.0);
          })
          ->rounding(palette ? palette->m_vars.bigRounding : 10)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  root->addChild(mainBg);

  auto mainColumn =
      CColumnLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  mainBg->addChild(mainColumn);

  // Top Layout (contains Sidebar and Content Section)
  auto topLayout =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
          ->commence();
  topLayout->setGrow(true);
  mainColumn->addChild(topLayout);

  // Content Area Section (100% width)
  auto contentSection =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
	 

  auto contentLayout =
      CColumnLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  contentSection->addChild(contentLayout);

  m_tabContentWrapper =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  contentLayout->addChild(m_tabContentWrapper);
  topLayout->addChild(contentSection);

  // Playback Control Bar
  UI::Components::PlaybackBarContext pCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .togglePlayPause = [this] {
        Services::MPDManager::togglePlayPause([this] { updateStatus(); });
      },
      .prevTrack = [this] {
        Services::MPDManager::prevTrack([this] { updateStatus(); });
      },
      .nextTrack = [this] {
        Services::MPDManager::nextTrack([this] { updateStatus(); });
      },
      .onNavigationClick = [this](eViewMode mode) {
        switchViewMode(mode);
      }};
  m_playbackBar = std::make_unique<UI::Components::PlaybackBar>(pCtx);
  m_playbackBar->build(mainColumn);

  // Notification Manager Init
  m_notificationManager.init(root, m_window, m_backend, palette, fontFamily);

  // Views Init
  UI::Views::QueueViewContext qCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .playMpdSongId = [this](int songId) {
        Services::MPDManager::playSongId(songId, [this] { updateStatus(); });
      },
      .removeSongFromQueue = [this](int songId) {
        Services::MPDManager::removeSongFromQueue(songId, [this] { updateStatus(); });
      },
      .showPlaylistSelectionDialog = [this](const std::string &uri) {
        showPlaylistSelectionDialog(uri);
      },
      .showQueueAddItemDialog = [this] { showQueueAddItemDialog(); }};
  m_queueView = std::make_unique<UI::Views::QueueView>(qCtx);

  UI::Views::DatabaseViewContext dbCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .playSongFromUri = [this](const std::string &uri) {
        Services::MPDManager::playSongFromUri(
            uri, [this](const std::string &msg) { showNotification(msg); },
            [this] { updateStatus(); });
      },
      .addSongToQueue = [this](const std::string &uri) {
        Services::MPDManager::addSongToQueue(
            uri, [this](const std::string &msg) { showNotification(msg); },
            [this] { updateStatus(); });
      },
      .showPlaylistSelectionDialog = [this](const std::string &uri) {
        showPlaylistSelectionDialog(uri);
      },
      .showNotification = [this](const std::string &msg) { showNotification(msg); }};
  m_dbView = std::make_unique<UI::Views::DatabaseView>(dbCtx);

  UI::Views::PlaylistsViewContext plCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .playSongFromUri = [this](const std::string &uri) {
        Services::MPDManager::playSongFromUri(
            uri, [this](const std::string &msg) { showNotification(msg); },
            [this] { updateStatus(); });
      },
      .addSongToQueue = [this](const std::string &uri) {
        Services::MPDManager::addSongToQueue(
            uri, [this](const std::string &msg) { showNotification(msg); },
            [this] { updateStatus(); });
      },
      .showRenameDialog = [this](const std::string &oldName) { showRenameDialog(oldName); },
      .showCreatePlaylistDialog = [this] { showCreatePlaylistDialog(); },
      .showPlaylistAddItemDialog = [this](const std::string &plName) { showPlaylistAddItemDialog(plName); },
      .showPlaylistSelectionDialog = [this](const std::string &uri, int pos) {
        showPlaylistSelectionDialog(uri, pos);
      },
      .showNotification = [this](const std::string &msg) { showNotification(msg); },
      .updateStatus = [this] { updateStatus(); }};
  m_playlistsView = std::make_unique<UI::Views::PlaylistsView>(plCtx);

  UI::Views::YtDlpViewContext ytCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .playSongFromUri = [this](const std::string &uri) {
        Services::MPDManager::playSongFromUri(
            uri, [this](const std::string &msg) { showNotification(msg); },
            [this] { updateStatus(); });
      },
      .addSongToQueue = [this](const std::string &uri) {
        Services::MPDManager::addSongToQueue(
            uri, [this](const std::string &msg) { showNotification(msg); },
            [this] { updateStatus(); });
      },
      .showPlaylistSelectionDialog = [this](const std::string &uri) {
        showPlaylistSelectionDialog(uri);
      },
      .showNotification = [this](const std::string &msg) { showNotification(msg); },
      .updateStatus = [this] { updateStatus(); },
      .getMusicDirectory = [this] { return getMusicDirectory(); }};
  m_ytDlpView = std::make_unique<UI::Views::YtDlpView>(ytCtx);

  UI::Views::SettingsViewContext sCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .mpdSettings = m_mpdSettings,
      .saveSettings = [this](const std::unordered_map<std::string, std::string> &newSet) {
        for (const auto &[k, v] : newSet) {
          m_mpdSettings[k] = v;
        }
        Services::SettingsManager::saveMpdConfig(
            Services::SettingsManager::getMpdConfPath(), m_mpdSettings);
      },
      .showNotification = [this](const std::string &msg) { showNotification(msg); }};
  m_settingsView = std::make_unique<UI::Views::SettingsView>(sCtx);

  UI::Views::PlayerViewContext playerCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      }};
  m_playerView = std::make_unique<UI::Views::PlayerView>(playerCtx);

  UI::Views::VisualizerViewContext visCtx{
      .backend = m_backend,
      .palette = palette};
  m_visualizerView = std::make_unique<UI::Views::VisualizerView>(visCtx);
}

void HyprMusicApp::setupEventHandlers() {
  m_window->m_events.closeRequest.listenStatic([this] {
    if (m_backend) {
      m_backend->addIdle([this] { m_backend->destroy(); });
    }
  });
}

void HyprMusicApp::switchViewMode(eViewMode mode) {
  if (m_viewMode == mode)
    return;
  if (m_viewMode == eViewMode::VIEW_VISUALIZER) {
    m_visualizerView->destroyVisualizer();
  }
  m_viewMode = mode;
  m_playlistLoaded = false;
  m_playlistsView->setSelectedPlaylist("");
  m_playlistsView->setDetailedView(false);
  m_playlistsView->resetLayout();
  m_dbView->resetLayout();
  m_queueView->resetLayout();
  m_settingsView->resetLayout();
  updateStatus();
}

void HyprMusicApp::showNotification(const std::string &msg) {
  m_notificationManager.showNotification(msg);
}

void HyprMusicApp::showRenameDialog(const std::string &oldName) {
  UI::Dialogs::RenamePlaylistContext ctx{
      .oldName = oldName,
      .parentWindow = m_window,
      .backend = m_backend,
      .palette = m_palette,
      .fontFamily = m_fontFamily,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .onRenamed = [this](const std::string &oldPl, const std::string &newPl) {
        if (m_playlistsView->getSelectedPlaylist() == oldPl) {
          m_playlistsView->setSelectedPlaylist(newPl);
        }
        m_backend->addTimer(
            std::chrono::milliseconds(100),
            [this](CAtomicSharedPointer<CTimer>, void *) {
              Services::MPDManager::runMpdCommand([this](struct mpd_connection *conn) {
                m_playlistsView->rebuildLeftItems(conn);
                m_playlistsView->rebuildRightItems(conn);
              });
            },
            nullptr);
      }};
  UI::Dialogs::showRenamePlaylistDialog(ctx);
}

void HyprMusicApp::showCreatePlaylistDialog() {
  UI::Dialogs::CreatePlaylistContext ctx{
      .parentWindow = m_window,
      .backend = m_backend,
      .palette = m_palette,
      .fontFamily = m_fontFamily,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .showNotification = [this](const std::string &msg) { showNotification(msg); },
      .onCreated = [this](const std::string &plName) {
        m_playlistsView->setSelectedPlaylist(plName);
        m_playlistsView->setDetailedView(true);
        m_playlistLoaded = false;
        m_backend->addTimer(
            std::chrono::milliseconds(100),
            [this, plName](CAtomicSharedPointer<CTimer>, void *) {
              showNotification("Created " + plName);
              updateStatus();
            },
            nullptr);
      }};
  UI::Dialogs::showCreatePlaylistDialog(ctx);
}

void HyprMusicApp::showPlaylistSelectionDialog(const std::string &songUri,
                                                int moveFromSongPos) {
  UI::Dialogs::PlaylistSelectionContext ctx{
      .songUri = songUri,
      .moveFromSongPos = moveFromSongPos,
      .currentSelectedPlaylist = m_playlistsView->getSelectedPlaylist(),
      .parentWindow = m_window,
      .backend = m_backend,
      .palette = m_palette,
      .fontFamily = m_fontFamily,
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
        Services::MPDManager::runMpdCommand(cmd);
      },
      .showNotification = [this](const std::string &msg) { showNotification(msg); },
      .onPlaylistUpdated = [this] {
        m_backend->addTimer(
            std::chrono::milliseconds(100),
            [this](CAtomicSharedPointer<CTimer>, void *) {
              updateStatus();
              if (m_viewMode == eViewMode::VIEW_PLAYLISTS) {
                Services::MPDManager::runMpdCommand([this](struct mpd_connection *conn) {
                  m_playlistsView->rebuildRightItems(conn);
                });
              }
            },
            nullptr);
      }};
  UI::Dialogs::showPlaylistSelectionDialog(ctx);
}

void HyprMusicApp::showQueueAddItemDialog() {
  UI::Dialogs::AddItemDialogContext ctx;
  ctx.targetType = UI::Dialogs::AddItemTargetType::QUEUE;
  ctx.window = m_window;
  ctx.backend = m_backend;
  ctx.palette = m_palette;
  ctx.fontFamily = m_fontFamily;
  ctx.runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
    Services::MPDManager::runMpdCommand(cmd);
  };
  ctx.showNotification = [this](const std::string &msg) { showNotification(msg); };
  ctx.addSongToQueue = [this](const std::string &uri) {
    Services::MPDManager::addSongToQueue(
        uri, [this](const std::string &m) { showNotification(m); },
        [this] { updateStatus(); });
  };
  ctx.refreshCallback = [this] { updateStatus(); };
  UI::Dialogs::showAddItemDialog(ctx);
}

void HyprMusicApp::showPlaylistAddItemDialog(const std::string &playlistName) {
  UI::Dialogs::AddItemDialogContext ctx;
  ctx.targetType = UI::Dialogs::AddItemTargetType::PLAYLIST;
  ctx.targetPlaylistName = playlistName;
  ctx.window = m_window;
  ctx.backend = m_backend;
  ctx.palette = m_palette;
  ctx.fontFamily = m_fontFamily;
  ctx.runMpdCommand = [](const std::function<void(struct mpd_connection *)> &cmd) {
    Services::MPDManager::runMpdCommand(cmd);
  };
  ctx.showNotification = [this](const std::string &msg) { showNotification(msg); };
  ctx.addSongToQueue = [this](const std::string &uri) {
    Services::MPDManager::addSongToQueue(
        uri, [this](const std::string &m) { showNotification(m); },
        [this] { updateStatus(); });
  };
  ctx.refreshCallback = [this] {
    Services::MPDManager::runMpdCommand([this](struct mpd_connection *conn) {
      m_playlistsView->rebuildRightItems(conn);
    });
  };
  UI::Dialogs::showAddItemDialog(ctx);
}

void HyprMusicApp::updateStatus() {
  struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);
  if (!conn)
    return;

  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    mpd_connection_free(conn);
    return;
  }

  std::string trackTitle = "No currently playing songs";
  std::string trackArtist = "";
  std::string stateText = "media-playback-start-symbolic";
  std::string currentSongUri = ""; 
  int activeSongId = -1;
  unsigned currentQueueVersion = 0;
  int currentVolume = -1;
  unsigned elapsed = 0;
  unsigned total = 100;
  bool hasActiveTrack = false;
  m_isPlaying = false;

  struct mpd_status *status = mpd_run_status(conn);
  if (status) {
    enum mpd_state state = mpd_status_get_state(status);
    activeSongId = mpd_status_get_song_id(status);
    currentQueueVersion = mpd_status_get_queue_version(status);
    currentVolume = mpd_status_get_volume(status);
    m_isPlaying = (state == MPD_STATE_PLAY);

    if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
      stateText = (state == MPD_STATE_PLAY) ? "media-playback-pause-symbolic" : "media-playback-start-symbolic";
      elapsed = mpd_status_get_elapsed_time(status);
      total = mpd_status_get_total_time(status);
      hasActiveTrack = true;

      struct mpd_song *song = mpd_run_current_song(conn);
      if (song) {
        const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        const char *uri = mpd_song_get_uri(song);
        if (uri)
          currentSongUri = uri;

        std::string storedTitle, storedUploader;
        if (title && strlen(title) > 0) {
          trackTitle = title;
          trackArtist = artist ? artist : "Unknown Artist";
        } else if (uri && m_ytDlpService.getUrlTitle(uri, storedTitle, storedUploader)) {
          trackTitle = "Stream (" + storedTitle + ")";
          trackArtist = storedUploader.empty() ? "" : storedUploader;
        } else if (uri) {
          std::string uriStr(uri);
          if (uriStr.find("googlevideo.com") != std::string::npos ||
              uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
            if (uriStr.length() > 50) {
              trackTitle = "🌐 Stream (" + uriStr.substr(0, 35) + "...)";
            } else {
              trackTitle = uriStr;
            }
          } else {
            trackTitle = uriStr;
          }
          trackArtist = "";
        } else {
          trackTitle = "Unknown track";
          trackArtist = "";
        }
        mpd_song_free(song);
      } else {
        trackTitle = "Unknown track";
        trackArtist = "";
      }

    } else {
      trackTitle = "No currently playing songs";
      trackArtist = "";
      stateText = "media-playback-start-symbolic";
    }

    if (m_viewMode == eViewMode::VIEW_QUEUE) {
      if (!m_playlistLoaded || m_lastQueueVersion != currentQueueVersion) {
        m_lastQueueVersion = currentQueueVersion;
        m_lastActiveSongId = activeSongId;
        m_playlistLoaded = true;
        m_queueView->rebuildUI(m_tabContentWrapper, conn, activeSongId);
      } else if (m_lastActiveSongId != activeSongId) {
        m_lastActiveSongId = activeSongId;
        m_queueView->setActiveSongId(activeSongId);
      }
    } else if (m_viewMode == eViewMode::VIEW_DATABASE) {
      if (!m_playlistLoaded) {
        m_playlistLoaded = true;
        m_dbView->rebuildUI(m_tabContentWrapper, conn);
      }
    } else if (m_viewMode == eViewMode::VIEW_PLAYLISTS) {
      if (!m_playlistLoaded) {
        m_playlistLoaded = true;
        m_playlistsView->rebuildUI(m_tabContentWrapper, conn);
      }
    } else if (m_viewMode == eViewMode::VIEW_YTDLP) {
      bool needRebuild = m_ytDlpView->needRebuild();
      if (needRebuild) {
        m_ytDlpView->clearNeedRebuild();
      }
      if (!m_playlistLoaded || needRebuild) {
        m_playlistLoaded = true;
        m_ytDlpView->rebuildUI(m_tabContentWrapper, conn);
      }
    } else if (m_viewMode == eViewMode::VIEW_SETTINGS) {
      if (!m_playlistLoaded) {
        m_playlistLoaded = true;
        m_settingsView->rebuildUI(m_tabContentWrapper);
      }
    } else if (m_viewMode == eViewMode::VIEW_VISUALIZER) {
      if (!m_playlistLoaded) {
        m_playlistLoaded = true;
        m_visualizerView->rebuildUI(m_tabContentWrapper);
      }
    } else if (m_viewMode == eViewMode::VIEW_PLAYER) {
      if (!m_playlistLoaded) {
        m_playlistLoaded = true;
        m_playerView->rebuildUI(m_tabContentWrapper, conn);
      }
      m_playerView->updateTrackInfo(trackTitle, hasActiveTrack, elapsed, total, currentSongUri);
    }

    mpd_status_free(status);
  }

  mpd_connection_free(conn);

  m_playbackBar->updateTrackInfo(trackTitle, trackArtist, hasActiveTrack, elapsed, total);
  m_playbackBar->updatePlayPauseState(stateText);
  m_playbackBar->updateVolume(currentVolume);
  m_playbackBar->updateAlbumArt(currentSongUri);
}

void HyprMusicApp::setupTimer() {
  m_backend->addTimer(
      std::chrono::seconds(1),
      [this](CAtomicSharedPointer<CTimer>, void *) {
        updateStatus();
        setupTimer();
      },
      nullptr);
}

std::string HyprMusicApp::getMusicDirectory() const {
  auto it = m_mpdSettings.find("music_directory");
  if (it != m_mpdSettings.end() && !it->second.empty()) {
    return expandTilde(it->second);
  }
  return getUserHomeDir() + "/Music";
}

} // namespace Core

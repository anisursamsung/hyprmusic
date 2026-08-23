#include "HlMusicApp.hpp"
#include "Utils/StreamUtils.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

namespace Core {

HlMusicApp::HlMusicApp(int argc, char *argv[]) {
  m_backend = IBackend::create();
  if (!m_backend) {
    throw std::runtime_error("Failed to create backend");
  }
  if (argc > 1 && argv != nullptr) {
    for (int i = 1; i < argc; ++i) {
      if (argv[i] && strlen(argv[i]) > 0) {
        m_cmdArgs.push_back(argv[i]);
      }
    }
  }
}

HlMusicApp::~HlMusicApp() {}

void HlMusicApp::run() {
  Services::MPDManager::ensureMpdRunningAndConfigured(m_mpdSettings);
  m_ipcService.init([this](const std::vector<std::string> &uris) {
    processUriArgs(uris);
  });
  createWindow();
  createUI();
  setupEventHandlers();
  std::cout << "Starting HlMusic..." << std::endl;
  m_window->open();
  updateStatus();
  setupTimer();
  processCommandLineArgs();
  m_backend->enterLoop();
}

void HlMusicApp::createWindow() {
  m_window = CWindowBuilder::begin()
                 ->type(HT_WINDOW_TOPLEVEL)
                 ->appTitle("HlMusic")
                 ->appClass("hlmusic")
                 ->preferredSize({0, 0})
                 ->minSize({600, 400})
                 ->commence();
  if (!m_window) {
    throw std::runtime_error("Failed to create window");
  }
}

void HlMusicApp::createUI() {
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

  // Top Layout
  auto topLayout =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
          ->commence();
  topLayout->setGrow(true);
  mainColumn->addChild(topLayout);

  // Content Area Section
  auto contentSection =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
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

  // Dialog Coordinator Init
  UI::Dialogs::DialogCoordinatorContext diagCtx{
      .parentWindow = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          },
      .showNotification =
          [this](const std::string &msg) { showNotification(msg); },
      .updateStatus = [this] { updateStatus(); }};
  m_dialogCoordinator =
      std::make_unique<UI::Dialogs::DialogCoordinator>(diagCtx);

  // Playback Control Bar Init
  UI::Components::PlaybackBarContext pCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          },
      .togglePlayPause =
          [this] {
            Services::MPDManager::togglePlayPause([this] { updateStatus(); });
          },
      .prevTrack =
          [this] {
            Services::MPDManager::prevTrack([this] { updateStatus(); });
          },
      .nextTrack =
          [this] {
            Services::MPDManager::nextTrack([this] { updateStatus(); });
          },
      .onNavigationClick = [this](eViewMode mode) { switchViewMode(mode); },
      .onRepeatModeChange =
          [this](bool repeat, bool single) {
            Services::MPDManager::runMpdCommand(
                [repeat, single](struct mpd_connection *conn) {
                  if (conn) {
                    mpd_run_repeat(conn, repeat);
                    mpd_run_single(conn, single);
                  }
                });
            if (repeat && single) {
              showNotification("Repeat Track is turned on");
            } else if (repeat) {
              showNotification("Repeat Queue is turned on");
            } else {
              showNotification("Repeat is turned off");
            }
          },
      .onRandomModeChange =
          [this](bool random) {
            Services::MPDManager::runMpdCommand(
                [random](struct mpd_connection *conn) {
                  if (conn) {
                    mpd_run_random(conn, random);
                  }
                });
            showNotification(random ? "Shuffle is turned on"
                                    : "Shuffle is turned off");
          },
      .onConsumeModeChange =
          [this](bool consume) {
            Services::MPDManager::runMpdCommand(
                [consume](struct mpd_connection *conn) {
                  if (conn) {
                    mpd_run_consume(conn, consume);
                  }
                });
            showNotification(consume ? "Consume is turned on"
                                     : "Consume is turned off");
          }};
  m_playbackBar = std::make_unique<UI::Components::PlaybackBar>(pCtx);
  m_playbackBar->setActiveViewMode(m_viewMode);
  m_playbackBar->build(mainColumn);

  // Views Init
  UI::Views::QueueViewContext qCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .dialogCoordinator = m_dialogCoordinator.get(),
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          },
      .playMpdSongId =
          [this](int songId) {
            Services::MPDManager::playSongId(songId,
                                             [this] { updateStatus(); });
          },
      .removeSongFromQueue =
          [this](int songId) {
            Services::MPDManager::removeSongFromQueue(
                songId, [this] { updateStatus(); });
          },
      .showPlaylistSelectionDialog =
          [this](const std::string &uri) {
            m_dialogCoordinator->showPlaylistSelectionDialog(
                uri, m_playlistsView->getSelectedPlaylist());
          },
      .showQueueAddItemDialog =
          [this] { m_dialogCoordinator->showQueueAddItemDialog(); },
      .showClearQueueDialog =
          [this](std::function<void()> onCleared) {
            m_dialogCoordinator->showClearQueueDialog(onCleared);
          }};
  m_queueView = std::make_unique<UI::Views::QueueView>(qCtx);

  UI::Views::DatabaseViewContext dbCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .dialogCoordinator = m_dialogCoordinator.get(),
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          },
      .playSongFromUri =
          [this](const std::string &uri) {
            Services::MPDManager::playSongFromUri(
                uri, [this](const std::string &msg) { showNotification(msg); },
                [this] { updateStatus(); });
          },
      .addSongToQueue =
          [this](const std::string &uri) {
            Services::MPDManager::addSongToQueue(
                uri, [this](const std::string &msg) { showNotification(msg); },
                [this] { updateStatus(); });
          },
      .showPlaylistSelectionDialog =
          [this](const std::string &uri) {
            m_dialogCoordinator->showPlaylistSelectionDialog(
                uri, m_playlistsView->getSelectedPlaylist());
          },
      .showNotification =
          [this](const std::string &msg) { showNotification(msg); }};
  m_dbView = std::make_unique<UI::Views::DatabaseView>(dbCtx);

  UI::Views::PlaylistsViewContext plCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .dialogCoordinator = m_dialogCoordinator.get(),
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          },
      .playSongFromUri =
          [this](const std::string &uri) {
            Services::MPDManager::playSongFromUri(
                uri, [this](const std::string &msg) { showNotification(msg); },
                [this] { updateStatus(); });
          },
      .addSongToQueue =
          [this](const std::string &uri) {
            Services::MPDManager::addSongToQueue(
                uri, [this](const std::string &msg) { showNotification(msg); },
                [this] { updateStatus(); });
          },
      .showRenameDialog =
          [this](const std::string &oldName) {
            m_dialogCoordinator->showRenameDialog(
                oldName, [this](const std::string &oldPl,
                                const std::string &newPl) {
                  if (m_playlistsView->getSelectedPlaylist() == oldPl) {
                    m_playlistsView->setSelectedPlaylist(newPl);
                  }
                  m_backend->addTimer(
                      std::chrono::milliseconds(100),
                      [this](CAtomicSharedPointer<CTimer>, void *) {
                        Services::MPDManager::runMpdCommand(
                            [this](struct mpd_connection *conn) {
                              m_playlistsView->rebuildLeftItems(conn);
                              m_playlistsView->rebuildRightItems(conn);
                            });
                      },
                      nullptr);
                });
          },
      .showCreatePlaylistDialog =
          [this] {
            m_dialogCoordinator->showCreatePlaylistDialog(
                [this](const std::string &plName) {
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
                });
          },
      .showDeletePlaylistDialog =
          [this](const std::string &plName, std::function<void()> onDeleted) {
            m_dialogCoordinator->showDeletePlaylistDialog(plName, onDeleted);
          },
      .showPlaylistAddItemDialog =
          [this](const std::string &plName) {
            m_dialogCoordinator->showPlaylistAddItemDialog(
                plName, [this] {
                  Services::MPDManager::runMpdCommand(
                      [this](struct mpd_connection *conn) {
                        m_playlistsView->rebuildRightItems(conn);
                      });
                });
          },
      .showPlaylistSelectionDialog =
          [this](const std::string &uri, int pos) {
            m_dialogCoordinator->showPlaylistSelectionDialog(
                uri, m_playlistsView->getSelectedPlaylist(), pos, [this] {
                  m_backend->addTimer(
                      std::chrono::milliseconds(100),
                      [this](CAtomicSharedPointer<CTimer>, void *) {
                        updateStatus();
                        if (m_viewMode == eViewMode::VIEW_PLAYLISTS) {
                          Services::MPDManager::runMpdCommand(
                              [this](struct mpd_connection *conn) {
                                m_playlistsView->rebuildRightItems(conn);
                              });
                        }
                      },
                      nullptr);
                });
          },
      .showNotification =
          [this](const std::string &msg) { showNotification(msg); },
      .updateStatus = [this] { updateStatus(); }};
  m_playlistsView = std::make_unique<UI::Views::PlaylistsView>(plCtx);

  UI::Views::YtDlpViewContext ytCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .dialogCoordinator = m_dialogCoordinator.get(),
      .ytDlpService = &m_ytDlpService,
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          },
      .playSongFromUri =
          [this](const std::string &uri) {
            Services::MPDManager::playSongFromUri(
                uri, [this](const std::string &msg) { showNotification(msg); },
                [this] { updateStatus(); });
          },
      .addSongToQueue =
          [this](const std::string &uri) {
            Services::MPDManager::addSongToQueue(
                uri, [this](const std::string &msg) { showNotification(msg); },
                [this] { updateStatus(); });
          },
      .showPlaylistSelectionDialog =
          [this](const std::string &uri) {
            m_dialogCoordinator->showPlaylistSelectionDialog(
                uri, m_playlistsView->getSelectedPlaylist());
          },
      .showNotification =
          [this](const std::string &msg) { showNotification(msg); },
      .updateStatus = [this] { updateStatus(); },
      .getMusicDirectory = [this] { return getMusicDirectory(); }};
  m_ytDlpView = std::make_unique<UI::Views::YtDlpView>(ytCtx);

  UI::Views::SettingsViewContext sCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .mpdSettings = m_mpdSettings,
      .saveSettings =
          [this](const std::unordered_map<std::string, std::string> &newSet) {
            for (const auto &[k, v] : newSet) {
              m_mpdSettings[k] = v;
            }
            Services::SettingsManager::saveMpdConfig(
                Services::SettingsManager::getMpdConfPath(), m_mpdSettings);
          },
      .showNotification =
          [this](const std::string &msg) { showNotification(msg); }};
  m_settingsView = std::make_unique<UI::Views::SettingsView>(sCtx);

  UI::Views::PlayerViewContext playerCtx{
      .window = m_window,
      .backend = m_backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .runMpdCommand =
          [](const std::function<void(struct mpd_connection *)> &cmd) {
            Services::MPDManager::runMpdCommand(cmd);
          }};
  m_playerView = std::make_unique<UI::Views::PlayerView>(playerCtx);

  UI::Views::VisualizerViewContext visCtx{.backend = m_backend,
                                          .palette = palette};
  m_visualizerView = std::make_unique<UI::Views::VisualizerView>(visCtx);
}

void HlMusicApp::setupEventHandlers() {
  m_window->m_events.closeRequest.listenStatic([this] {
    if (m_backend) {
      m_backend->addIdle([this] { m_backend->destroy(); });
    }
  });
}

void HlMusicApp::switchViewMode(eViewMode mode) {
  if (m_viewMode == mode)
    return;
  if (m_dialogCoordinator) {
    m_dialogCoordinator->closeActiveDialog();
  }
  if (m_viewMode == eViewMode::VIEW_VISUALIZER) {
    m_visualizerView->destroyVisualizer();
  }
  m_viewMode = mode;
  if (m_playbackBar) {
    m_playbackBar->setActiveViewMode(mode);
  }
  m_playlistLoaded = false;
  m_playlistsView->setSelectedPlaylist("");
  m_playlistsView->setDetailedView(false);
  m_playlistsView->resetLayout();
  m_dbView->resetLayout();
  m_queueView->resetLayout();
  m_settingsView->resetLayout();
  updateStatus();
}

void HlMusicApp::showNotification(const std::string &msg) {
  m_notificationManager.showNotification(msg);
}

void HlMusicApp::updateStatus() {
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
  bool isRepeat = false;
  bool isSingle = false;
  bool isRandom = false;
  bool isConsume = false;

  struct mpd_status *status = mpd_run_status(conn);
  if (status) {
    enum mpd_state state = mpd_status_get_state(status);
    activeSongId = mpd_status_get_song_id(status);
    currentQueueVersion = mpd_status_get_queue_version(status);
    currentVolume = mpd_status_get_volume(status);
    isRepeat = mpd_status_get_repeat(status);
    isSingle = mpd_status_get_single(status);
    isRandom = mpd_status_get_random(status);
    isConsume = mpd_status_get_consume(status);

    if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
      stateText = (state == MPD_STATE_PLAY) ? "media-playback-pause-symbolic"
                                            : "media-playback-start-symbolic";
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
        } else if (uri && m_ytDlpService.getUrlTitle(uri, storedTitle,
                                                     storedUploader)) {
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
      m_playerView->updateTrackInfo(trackTitle, hasActiveTrack, elapsed, total,
                                    currentSongUri);
    }

    mpd_status_free(status);
  }

  mpd_connection_free(conn);

  m_playbackBar->updateTrackInfo(trackTitle, trackArtist, hasActiveTrack,
                                 elapsed, total);
  m_playbackBar->updatePlayPauseState(stateText);
  m_playbackBar->updateRepeatState(isRepeat, isSingle);
  m_playbackBar->updateRandomState(isRandom);
  m_playbackBar->updateConsumeState(isConsume);
  m_playbackBar->updateVolume(currentVolume);
  m_playbackBar->updateAlbumArt(currentSongUri);
}

void HlMusicApp::setupTimer() {
  m_backend->addTimer(
      std::chrono::seconds(1),
      [this](CAtomicSharedPointer<CTimer>, void *) {
        updateStatus();
        m_ipcService.poll();
        setupTimer();
      },
      nullptr);
}

std::string HlMusicApp::getMusicDirectory() const {
  auto it = m_mpdSettings.find("music_directory");
  if (it != m_mpdSettings.end() && !it->second.empty()) {
    return expandTilde(it->second);
  }
  return getUserHomeDir() + "/Music";
}

std::string HlMusicApp::resolveToMpdUri(const std::string &pathOrUri) const {
  if (pathOrUri.empty())
    return "";

  if (pathOrUri.rfind("http://", 0) == 0 ||
      pathOrUri.rfind("https://", 0) == 0) {
    return pathOrUri;
  }

  std::string filePath = pathOrUri;
  if (filePath.rfind("file://", 0) == 0) {
    filePath = filePath.substr(7);
  }

  std::filesystem::path p(filePath);
  std::error_code ec;
  std::filesystem::path absP = std::filesystem::absolute(p, ec);
  if (ec) {
    absP = p;
  }

  std::string musicDir = getMusicDirectory();
  std::filesystem::path musicDirP(musicDir);

  std::filesystem::path relP = absP.lexically_relative(musicDirP);
  if (!relP.empty() && relP.native().rfind("..", 0) != 0 && relP.is_relative()) {
    return relP.string();
  }

  return "file://" + absP.string();
}

void HlMusicApp::processCommandLineArgs() {
  if (m_cmdArgs.empty())
    return;
  processUriArgs(m_cmdArgs);
}

void HlMusicApp::processUriArgs(const std::vector<std::string> &args) {
  if (args.empty())
    return;

  std::vector<std::string> targetUris;
  std::unordered_set<std::string> seen;
  for (const auto &arg : args) {
    if (arg.empty())
      continue;
    std::string uri = resolveToMpdUri(arg);
    if (!uri.empty() && seen.find(uri) == seen.end()) {
      seen.insert(uri);
      targetUris.push_back(uri);
    }
  }

  if (targetUris.empty())
    return;

  if (targetUris.size() == 1) {
    const std::string uri = targetUris[0];
    Services::MPDManager::playSongFromUri(
        uri,
        [this](const std::string &msg) { showNotification(msg); },
        nullptr);
    switchViewMode(eViewMode::VIEW_QUEUE);
    updateStatus();
    m_playbackBar->forceUpdateAlbumArt(uri);
    return;
  }

  Services::MPDManager::runMpdCommand(
      [this, targetUris](struct mpd_connection *conn) {
        if (!conn)
          return;

        std::unordered_set<std::string> queueUris;
        std::unordered_map<std::string, int> queueSongIds;

        if (mpd_send_list_queue_meta(conn)) {
          struct mpd_song *s;
          while ((s = mpd_recv_song(conn)) != NULL) {
            const char *qUri = mpd_song_get_uri(s);
            int sId = mpd_song_get_id(s);
            if (qUri) {
              queueUris.insert(qUri);
              queueSongIds[qUri] = sId;
            }
            mpd_song_free(s);
          }
          mpd_response_finish(conn);
        }

        int addedCount = 0;
        int skippedCount = 0;
        int firstPlayId = -1;

        const std::string &firstUri = targetUris[0];
        auto it = queueSongIds.find(firstUri);
        if (it != queueSongIds.end()) {
          firstPlayId = it->second;
          skippedCount++;
        } else {
          int newId = mpd_run_add_id(conn, firstUri.c_str());
          if (newId >= 0) {
            firstPlayId = newId;
            addedCount++;
            queueUris.insert(firstUri);
          }
        }

        for (size_t i = 1; i < targetUris.size(); ++i) {
          const std::string &uri = targetUris[i];
          if (queueUris.find(uri) != queueUris.end()) {
            skippedCount++;
          } else {
            if (mpd_run_add(conn, uri.c_str())) {
              addedCount++;
              queueUris.insert(uri);
            }
          }
        }

        if (firstPlayId >= 0) {
          mpd_run_play_id(conn, firstPlayId);
        }

        if (addedCount > 0 && skippedCount > 0) {
          showNotification("Added " + std::to_string(addedCount) + " tracks (" +
                           std::to_string(skippedCount) +
                           " skipped as duplicates), playing 1st track");
        } else if (addedCount > 0) {
          showNotification("Added " + std::to_string(addedCount) +
                           " tracks to queue, playing 1st track");
        } else if (skippedCount > 0) {
          showNotification("All tracks already in queue, playing 1st track");
        }
      });

  switchViewMode(eViewMode::VIEW_QUEUE);
  updateStatus();
  if (!targetUris.empty()) {
    m_playbackBar->forceUpdateAlbumArt(targetUris[0]);
  }
}

} // namespace Core

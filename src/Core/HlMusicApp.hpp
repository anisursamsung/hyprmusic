#pragma once

#include "Core/IpcService.hpp"
#include "Core/ViewMode.hpp"
#include "Dialogs/DialogCoordinator.hpp"
#include "MPD/MPDManager.hpp"
#include "PlaybackBar/PlaybackBar.hpp"
#include "Tabs/Database/DatabaseView.hpp"
#include "Tabs/Player/PlayerView.hpp"
#include "Tabs/Playlists/PlaylistsView.hpp"
#include "Tabs/Queue/QueueView.hpp"
#include "Tabs/Settings/SettingsManager.hpp"
#include "Tabs/Settings/SettingsView.hpp"
#include "Tabs/Visualizer/VisualizerView.hpp"
#include "Tabs/YtDlp/YtDlpService.hpp"
#include "Tabs/YtDlp/YtDlpView.hpp"
#include "Utils/NotificationManager.hpp"
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class HlMusicApp {
public:
  HlMusicApp(int argc = 0, char *argv[] = nullptr);
  ~HlMusicApp();
  void run();

private:
  void createWindow();
  void createUI();
  void setupEventHandlers();
  void setupTimer();
  void processCommandLineArgs();
  void processUriArgs(const std::vector<std::string> &uris);
  std::string resolveToMpdUri(const std::string &pathOrUri) const;

  void switchViewMode(eViewMode mode);
  void updateStatus();
  void showNotification(const std::string &msg);
  std::string getMusicDirectory() const;

  CSharedPointer<IBackend> m_backend;
  CSharedPointer<IWindow> m_window;
  CSharedPointer<CPalette> m_palette;
  std::string m_fontFamily = "Sans Serif";

  eViewMode m_viewMode = eViewMode::VIEW_PLAYER;

  std::unordered_map<std::string, std::string> m_mpdSettings;
  Services::YtDlpService m_ytDlpService;
  Services::IpcService m_ipcService;

  Utils::NotificationManager m_notificationManager;
  std::unique_ptr<UI::Components::PlaybackBar> m_playbackBar;
  std::unique_ptr<UI::Dialogs::DialogCoordinator> m_dialogCoordinator;

  std::unique_ptr<UI::Views::QueueView> m_queueView;
  std::unique_ptr<UI::Views::DatabaseView> m_dbView;
  std::unique_ptr<UI::Views::PlaylistsView> m_playlistsView;
  std::unique_ptr<UI::Views::YtDlpView> m_ytDlpView;
  std::unique_ptr<UI::Views::SettingsView> m_settingsView;
  std::unique_ptr<UI::Views::PlayerView> m_playerView;
  std::unique_ptr<UI::Views::VisualizerView> m_visualizerView;

  CSharedPointer<CRectangleElement> m_tabContentWrapper;

  unsigned m_lastQueueVersion = 0;
  int m_lastActiveSongId = -2;
  bool m_playlistLoaded = false;
  std::vector<std::string> m_cmdArgs;
};

} // namespace Core

#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include "ViewMode.hpp"
#include "../Services/MPDManager.hpp"
#include "../Services/SettingsManager.hpp"
#include "../Services/YtDlpService.hpp"
#include "../UI/Components/NotificationManager.hpp"
#include "../UI/Components/PlaybackBar.hpp"
#include "../UI/Components/TabBar.hpp"
#include "../UI/Views/DatabaseView.hpp"
#include "../UI/Views/HelpView.hpp"
#include "../UI/Views/PlaylistsView.hpp"
#include "../UI/Views/QueueView.hpp"
#include "../UI/Views/SettingsView.hpp"
#include "../UI/Views/TestView.hpp"
#include "../UI/Views/YtDlpView.hpp"

namespace Core {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class HyprMusicApp {
public:
  HyprMusicApp();
  void run();

private:
  void createWindow();
  void createUI();
  void setupEventHandlers();
  void setupTimer();

  void switchViewMode(eViewMode mode);
  void updateStatus();
  void showNotification(const std::string &msg);
  std::string getMusicDirectory() const;

  // Dialog helpers
  void showRenameDialog(const std::string &oldName);
  void showCreatePlaylistDialog();
  void showPlaylistSelectionDialog(const std::string &songUri, int moveFromSongPos = -1);
  void showQueueAddItemDialog();
  void showPlaylistAddItemDialog(const std::string &playlistName);

  CSharedPointer<IBackend> m_backend;
  CSharedPointer<IWindow> m_window;
  CSharedPointer<CPalette> m_palette;
  std::string m_fontFamily = "Sans Serif";

  eViewMode m_viewMode = eViewMode::VIEW_QUEUE;

  std::unordered_map<std::string, std::string> m_mpdSettings;
  Services::YtDlpService m_ytDlpService;

  UI::Components::NotificationManager m_notificationManager;
  std::unique_ptr<UI::Components::TabBar> m_tabBar;
  std::unique_ptr<UI::Components::PlaybackBar> m_playbackBar;

  std::unique_ptr<UI::Views::QueueView> m_queueView;
  std::unique_ptr<UI::Views::DatabaseView> m_dbView;
  std::unique_ptr<UI::Views::PlaylistsView> m_playlistsView;
  std::unique_ptr<UI::Views::YtDlpView> m_ytDlpView;
  std::unique_ptr<UI::Views::SettingsView> m_settingsView;
  std::unique_ptr<UI::Views::HelpView> m_helpView;
  std::unique_ptr<UI::Views::TestView> m_testView;

  CSharedPointer<CRectangleElement> m_tabContentWrapper;

  unsigned m_lastQueueVersion = 0;
  int m_lastActiveSongId = -2;
  bool m_playlistLoaded = false;
  bool m_isPlaying = false;
};

} // namespace Core

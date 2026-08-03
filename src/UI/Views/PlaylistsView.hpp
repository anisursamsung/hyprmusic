#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../Services/YtDlpService.hpp"
#include "../Components/SongCard.hpp"

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlaylistsViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;

  Services::YtDlpService *ytDlpService = nullptr;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
  std::function<void(const std::string &uri)> playSongFromUri;
  std::function<void(const std::string &uri)> addSongToQueue;
  std::function<void(const std::string &oldName)> showRenameDialog;
  std::function<void()> showCreatePlaylistDialog;
  std::function<void(const std::string &playlistName)> showPlaylistAddItemDialog;
  std::function<void(const std::string &songUri, int movePos)> showPlaylistSelectionDialog;
  std::function<void(const std::string &msg)> showNotification;
  std::function<void()> updateStatus;
};

class PlaylistsView {
public:
  explicit PlaylistsView(const PlaylistsViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn);
  void rebuildLeftItems(struct mpd_connection *conn);
  void rebuildRightItems(struct mpd_connection *conn);
  void layoutPlaylists();
  void addPlaylistToQueue(const std::string &plName);
  void resetLayout() {
    m_leftItemsLayout = nullptr;
    m_rightItemsLayout = nullptr;
  }

  void setSelectedPlaylist(const std::string &pl) { m_selectedPlaylist = pl; }
  void setDetailedView(bool detailed) { m_detailedView = detailed; }
  std::string getSelectedPlaylist() const { return m_selectedPlaylist; }

private:
  PlaylistsViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CColumnLayoutElement> m_leftItemsLayout;
  CSharedPointer<CColumnLayoutElement> m_rightItemsLayout;

  std::string m_searchQuery = "";
  std::string m_selectedPlaylist = "";
  bool m_detailedView = false;

  std::vector<std::string> m_currentPlaylists;
  std::unordered_map<std::string, int> m_playlistTrackCounts;
  double m_lastPlaylistsWidth = 0.0;
};

} // namespace UI::Views

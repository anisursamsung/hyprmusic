#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../Components/SongCard.hpp"

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct DatabaseViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
  std::function<void(const std::string &uri)> playSongFromUri;
  std::function<void(const std::string &uri)> addSongToQueue;
  std::function<void(const std::string &uri)> showPlaylistSelectionDialog;
  std::function<void(const std::string &msg)> showNotification;
};

class DatabaseView {
public:
  explicit DatabaseView(const DatabaseViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn);
  void populateDatabaseSongs(struct mpd_connection *conn);
  std::vector<std::string> collectMatchingSongUris(struct mpd_connection *conn);
  void resetLayout() { m_dbContentLayout = nullptr; }

private:
  DatabaseViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CColumnLayoutElement> m_dbContentLayout;

  std::string m_searchQuery = "";
  std::unordered_map<std::string, std::shared_ptr<UI::Components::SongCard>> m_dbSongCards;
};

} // namespace UI::Views

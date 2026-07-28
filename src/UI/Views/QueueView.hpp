#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "../../Services/YtDlpService.hpp"

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct QueueViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;

  Services::YtDlpService *ytDlpService = nullptr;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
  std::function<void(int songId)> playMpdSongId;
  std::function<void(int songId)> removeSongFromQueue;
  std::function<void(const std::string &songUri)> showPlaylistSelectionDialog;
  std::function<void()> showQueueAddItemDialog;
};

class QueueView {
public:
  explicit QueueView(const QueueViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn, int activeSongId);
  void populateQueueSongs(struct mpd_connection *conn, int activeSongId);
  void resetLayout() { m_queueContentLayout = nullptr; }

private:
  QueueViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CColumnLayoutElement> m_queueContentLayout;

  std::string m_searchQuery = "";
  int m_lastActiveSongId = -2;
  std::unordered_map<int, CSharedPointer<CTextElement>> m_queueSongTexts;
};

} // namespace UI::Views

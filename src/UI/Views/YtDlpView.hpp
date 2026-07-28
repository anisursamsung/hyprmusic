#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../../Services/YtDlpService.hpp"

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct YtDlpViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;

  Services::YtDlpService *ytDlpService = nullptr;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
  std::function<void(const std::string &uri)> playSongFromUri;
  std::function<void(const std::string &uri)> addSongToQueue;
  std::function<void(const std::string &songUri)> showPlaylistSelectionDialog;
  std::function<void(const std::string &msg)> showNotification;
  std::function<void()> updateStatus;
};

class YtDlpView {
public:
  explicit YtDlpView(const YtDlpViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn = nullptr);
  void triggerSearch();
  void loadYoutubePlaylist(const std::vector<Services::YtDlpResult> &tracks, const std::string &plName, bool playAfter);

  bool needRebuild() const { return m_ytdlpNeedRebuild; }
  void clearNeedRebuild() { m_ytdlpNeedRebuild = false; }

private:
  YtDlpViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;

  std::string m_searchTitle = "";
  std::string m_resultCount = "5";
  bool m_searching = false;
  bool m_ytdlpNeedRebuild = false;

  std::vector<Services::YtDlpResult> m_results;
  bool m_isPlaylist = false;
  std::string m_playlistTitle = "";
  std::string m_playlistId = "";
};

} // namespace UI::Views

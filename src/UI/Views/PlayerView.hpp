#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>

namespace UI::Views {
using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlayerViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
};

class PlayerView {
public:
  explicit PlayerView(const PlayerViewContext &ctx);
  void rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn);
  void updateTrackInfo(const std::string &trackText, bool hasActiveTrack, unsigned elapsed, unsigned total, const std::string &songUri);
  
private:
  PlayerViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  CSharedPointer<CImageElement> m_bgImage;
  CSharedPointer<CRectangleElement> m_detailsCard;
  CSharedPointer<CTextElement> m_titleText;
  CSharedPointer<CTextElement> m_artistAlbumText;
  CSharedPointer<CTextElement> m_yearGenreText;
  CSharedPointer<CTextElement> m_qualityText;
  CSharedPointer<CTextElement> m_trackDiscText;
  CSharedPointer<CTextElement> m_timeText;
  std::string m_lastSongUri;
  std::string m_currentArtPath;
};

} // namespace UI::Views

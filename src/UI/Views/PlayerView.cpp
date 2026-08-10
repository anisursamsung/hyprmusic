#include "PlayerView.hpp"
#include "../../Utils/ArtworkUtils.hpp"

namespace UI::Views {

PlayerView::PlayerView(const PlayerViewContext &ctx) : m_ctx(ctx) {}

void PlayerView::rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  m_tabContentWrapper->clearChildren();

  std::string currentUri = "";

  if (conn) {
    struct mpd_song *s = mpd_run_current_song(conn);
    if (s) {
      const char *u = mpd_song_get_uri(s);
      if (u)
        currentUri = u;
      mpd_song_free(s);
    }
  }
  m_lastSongUri = currentUri;

  std::string artPath = Utils::resolveTrackArtwork(conn, currentUri);
  if (artPath.empty()) {
    artPath = Utils::getDefaultArtworkPath();
  }

  // 1. Full-screen Artwork Backdrop
  if (!artPath.empty()) {
    m_bgImage = CImageBuilder::begin()
                    ->path(std::string(artPath))
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->rounding(0)
                    ->fitMode(IMAGE_FIT_MODE_COVER)
                    ->commence();
    m_bgImage->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    m_bgImage->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    m_tabContentWrapper->addChild(m_bgImage);
  }

  // 2. Dark Vignette Overlay
  auto vignette = CRectangleBuilder::begin()
                      ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.55F); })
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                          CDynamicSize::HT_SIZE_PERCENT,
                                          {1.0F, 1.0F}))
                      ->commence();
  vignette->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  vignette->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  m_tabContentWrapper->addChild(vignette);

  m_tabContentWrapper->forceReposition();
}

void PlayerView::updateTrackInfo(const std::string & /*trackText*/, bool /*hasActiveTrack*/,
                                unsigned /*elapsed*/, unsigned /*total*/, const std::string &songUri) {
  if (m_lastSongUri != songUri) {
    m_lastSongUri = songUri;

    m_ctx.runMpdCommand([this, songUri](struct mpd_connection *conn) {
      std::string artPath = Utils::resolveTrackArtwork(conn, songUri);
      if (artPath.empty()) {
        artPath = Utils::getDefaultArtworkPath();
      }

      if (m_bgImage) {
        m_bgImage->rebuild()->path(std::string(artPath))->commence();
      }
    });
  }
}

} // namespace UI::Views


#include "PlayerView.hpp"
#include "../../Utils/ArtworkUtils.hpp"

namespace UI::Views {
PlayerView::PlayerView(const PlayerViewContext &ctx) : m_ctx(ctx) {}

// 1. Foreground image (centered auto width, full height and rounding corner) 
static CSharedPointer<IElement> buildAlbumArtCard(const std::string &artPath) {
  auto artImage = CImageBuilder::begin()
                      ->path(std::string(artPath))
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                          CDynamicSize::HT_SIZE_PERCENT,
                                          {1.0F, 1.0F}))
                      ->rounding(0) 
                      ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                      ->commence();

  artImage->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  artImage->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  return artImage;
}

// 2. Overlay between the background and foreground
static CSharedPointer<IElement> buildDarkTintOverlay(CSharedPointer<CPalette> palette) {
  auto tintOverlay =
      CRectangleBuilder::begin()
          ->color([palette] {
            if (palette) {
              auto bg = palette->m_colors.background;
              return CHyprColor(bg.r, bg.g, bg.b, 0.55F); // 55% dark tint
            }
            return CHyprColor(0.12F, 0.12F, 0.12F, 0.55F);
          })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, 1.0F}))
          ->commence();
  tintOverlay->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  tintOverlay->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  return tintOverlay;
}

// 3. fullscreen background covering the entire view area
static CSharedPointer<IElement> buildFullscreenBackground(const std::string &artPath) {
  auto bgImage = CImageBuilder::begin()
                 ->path(std::string(artPath))
                 ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                     CDynamicSize::HT_SIZE_PERCENT,
                                     {1.0F, 1.0F}))
                 ->rounding(0)
                 ->fitMode(IMAGE_FIT_MODE_COVER) // Fills entire screen nicely
                 ->commence();
  bgImage->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  bgImage->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  return bgImage;
}

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

  // Assemble 
  m_coverCardElementBackground = buildFullscreenBackground(artPath);
  m_vignetteOverlay = buildDarkTintOverlay(m_ctx.palette);
  m_coverCardElement = buildAlbumArtCard(artPath);

  m_tabContentWrapper->addChild(m_coverCardElementBackground);
  m_tabContentWrapper->addChild(m_vignetteOverlay);
  m_tabContentWrapper->addChild(m_coverCardElement);
  m_tabContentWrapper->forceReposition();
}

void PlayerView::updateTrackInfo(const std::string & /*trackText*/, bool /*hasActiveTrack*/,
                               unsigned /*elapsed*/, unsigned /*total*/, const std::string &songUri) {
  if (m_lastSongUri != songUri) {
    m_lastSongUri = songUri;

    m_ctx.runMpdCommand([this, songUri](struct mpd_connection *conn) {
      std::string artPath = Utils::resolveTrackArtwork(conn, songUri);

      if (m_tabContentWrapper) {
        m_tabContentWrapper->clearChildren();

        m_coverCardElementBackground = buildFullscreenBackground(artPath);
        m_vignetteOverlay = buildDarkTintOverlay(m_ctx.palette);
        m_coverCardElement = buildAlbumArtCard(artPath);

        m_tabContentWrapper->addChild(m_coverCardElementBackground);
        m_tabContentWrapper->addChild(m_vignetteOverlay);
        m_tabContentWrapper->addChild(m_coverCardElement);
        m_tabContentWrapper->forceReposition();
      }
    });
  }
}

} // namespace UI::Views

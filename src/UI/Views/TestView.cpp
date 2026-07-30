#include "TestView.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/system/Icons.hpp>
#include <algorithm>

namespace UI::Views {

TestView::TestView(const TestViewContext &ctx) : m_ctx(ctx) {}

static CSharedPointer<IElement> buildCardElement(CSharedPointer<CPalette> palette,
                                                 CSharedPointer<IBackend> backend,
                                                 const std::string &artPath) {
  int rounding = 0;

  if (!artPath.empty()) {
    std::string artPathStr = artPath;
    auto img = CImageBuilder::begin()
                   ->path(std::string(artPathStr))
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 1.0F}))
                   ->rounding(rounding)
                   ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                   ->commence();
    img->setGrow(true, true);
    img->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    img->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    return img;
  }

  auto card = CRectangleBuilder::begin()
                  ->color([palette] {
                    return palette ? palette->m_colors.alternateBase
                                   : CHyprColor(0.12, 0.12, 0.12, 1.0);
                  })
                  ->rounding(rounding)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                      CDynamicSize::HT_SIZE_PERCENT,
                                      {1.0F, 1.0F}))
                  ->commence();
  card->setGrow(true, true);
  card->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  card->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  auto iconFactory = backend ? backend->systemIcons() : nullptr;
  CSharedPointer<ISystemIconDescription> musicIcon;
  if (iconFactory) {
    musicIcon = iconFactory->lookupIcon("media-optical-audio-symbolic");
    if (!musicIcon)
      musicIcon = iconFactory->lookupIcon("audio-x-generic");
  }

  CSharedPointer<IElement> iconElem;
  if (musicIcon) {
    iconElem = CImageBuilder::begin()
                   ->icon(musicIcon)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.6F, 0.6F}))
                   ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                   ->commence();
  } else {
    iconElem = CTextBuilder::begin()
                   ->text(std::string("🎵"))
                   ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
                   ->align(HT_FONT_ALIGN_CENTER)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                       CDynamicSize::HT_SIZE_AUTO,
                                       {1.0F, 1.0F}))
                   ->commence();
  }
  iconElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  iconElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  card->addChild(iconElem);
  return card;
}

void TestView::rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn) {
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
  m_coverCardElement = buildCardElement(m_ctx.palette, m_ctx.backend, artPath);

  m_tabContentWrapper->addChild(m_coverCardElement);
  m_tabContentWrapper->forceReposition();
}

void TestView::updateTrackInfo(const std::string & /*trackText*/, bool /*hasActiveTrack*/,
                               unsigned /*elapsed*/, unsigned /*total*/, const std::string &songUri) {
  if (m_lastSongUri != songUri) {
    m_lastSongUri = songUri;
    m_ctx.runMpdCommand([this, songUri](struct mpd_connection *conn) {
      std::string artPath = Utils::resolveTrackArtwork(conn, songUri);
      if (m_tabContentWrapper) {
        m_tabContentWrapper->clearChildren();
        m_coverCardElement = buildCardElement(m_ctx.palette, m_ctx.backend, artPath);
        m_tabContentWrapper->addChild(m_coverCardElement);
        m_tabContentWrapper->forceReposition();
      }
    });
  }
}

} // namespace UI::Views

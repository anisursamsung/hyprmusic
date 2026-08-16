#include "PlayerView.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace UI::Views {

static std::string formatTime(unsigned sec) {
  unsigned m = sec / 60;
  unsigned s = sec % 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "%02u:%02u", m, s);
  return std::string(buf);
}

PlayerView::PlayerView(const PlayerViewContext &ctx) : m_ctx(ctx) {}

void PlayerView::rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  m_tabContentWrapper->clearChildren();

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  std::string boldFont = fontFamily.empty() ? "Sans Serif Bold" : (fontFamily + " Bold");

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
  m_currentArtPath = artPath;

  // 1. Full-screen Artwork Backdrop
  if (!artPath.empty()) {
    m_bgImage = CImageBuilder::begin()
                    ->path(std::string(artPath))
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->rounding(0)
                    ->fitMode(IMAGE_FIT_MODE_COVER)
                    ->sync(true)
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

  // 3. Floating Glassmorphism Song Details Card Box
  m_detailsCard =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.10F, 0.10F, 0.14F, 0.85F);
          })
          ->rounding(16)
          ->borderThickness(1)
          ->borderColor([palette] {
            return palette ? palette->m_colors.text.mix(palette->m_colors.background, 0.85)
                           : CHyprColor(1.0F, 1.0F, 1.0F, 0.15F);
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO,
                              {0.60F, 1.0F}))
          ->commence();
  m_detailsCard->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_detailsCard->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  auto cardCol =
      CColumnLayoutBuilder::begin()
          ->gap(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO,
                              {1.0F, 1.0F}))
          ->commence();
  cardCol->setMargin(22);

  m_titleText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->fontFamily(std::string(boldFont))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H1, 1.5f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  m_artistAlbumText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text.mix(palette->m_colors.alternateBase, 0.20)
                           : CHyprColor(0.85F, 0.85F, 0.90F, 1.0F);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT, 1.2f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  m_yearGenreText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text.mix(palette->m_colors.alternateBase, 0.40)
                           : CHyprColor(0.70F, 0.70F, 0.75F, 1.0F);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT, 1.0f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  m_qualityText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.30F, 0.85F, 1.0F, 0.95F);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 1.05f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  m_trackDiscText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text.mix(palette->m_colors.alternateBase, 0.40)
                           : CHyprColor(0.65F, 0.65F, 0.70F, 1.0F);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.95f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  m_timeText =
      CTextBuilder::begin()
          ->text("")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT, 1.1f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  cardCol->addChild(m_titleText);
  cardCol->addChild(m_artistAlbumText);
  cardCol->addChild(m_yearGenreText);
  cardCol->addChild(m_qualityText);
  cardCol->addChild(m_trackDiscText);
  cardCol->addChild(m_timeText);

  m_detailsCard->addChild(cardCol);
  m_tabContentWrapper->addChild(m_detailsCard);

  m_tabContentWrapper->forceReposition();
}

void PlayerView::updateTrackInfo(const std::string &trackText, bool hasActiveTrack,
                                unsigned elapsed, unsigned total, const std::string &songUri) {
  if (!hasActiveTrack || songUri.empty()) {
    m_lastSongUri = "";
    m_currentArtPath = Utils::getDefaultArtworkPath();
    if (m_bgImage) {
      m_bgImage->rebuild()->path(std::string(m_currentArtPath))->commence();
    }
    if (m_detailsCard) {
      m_detailsCard->rebuild()->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })->borderThickness(0)->commence();
    }
    if (m_titleText) m_titleText->rebuild()->text("")->commence();
    if (m_artistAlbumText) m_artistAlbumText->rebuild()->text("")->commence();
    if (m_yearGenreText) m_yearGenreText->rebuild()->text("")->commence();
    if (m_qualityText) m_qualityText->rebuild()->text("")->commence();
    if (m_trackDiscText) m_trackDiscText->rebuild()->text("")->commence();
    if (m_timeText) m_timeText->rebuild()->text("")->commence();

    if (m_tabContentWrapper) {
      m_tabContentWrapper->forceReposition();
    }
    return;
  }

  // 1. Time string
  std::string timeStr = formatTime(elapsed) + " / " + formatTime(total);
  if (m_timeText) {
    m_timeText->rebuild()->text(std::string(timeStr))->commence();
  }

  // 2. Make details card visible with alternateBase palette color
  auto palette = m_ctx.palette;
  if (m_detailsCard) {
    m_detailsCard->rebuild()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.10F, 0.10F, 0.14F, 0.85F);
          })
          ->borderThickness(1)
          ->borderColor([palette] {
            return palette ? palette->m_colors.text.mix(palette->m_colors.background, 0.85)
                           : CHyprColor(1.0F, 1.0F, 1.0F, 0.15F);
          })
        ->commence();
  }

  // 3. Fetch tags & audio format specs from MPD
  m_ctx.runMpdCommand([this, songUri, trackText](struct mpd_connection *conn) {
    std::string titleStr = trackText;
    std::string artistAlbumStr = "";
    std::string yearGenreStr = "";
    std::string qualityStr = "";
    std::string trackDiscStr = "";

    if (conn) {
      struct mpd_song *song = mpd_run_current_song(conn);
      if (song) {
        const char *t = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        const char *a = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        const char *alb = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
        const char *d = mpd_song_get_tag(song, MPD_TAG_DATE, 0);
        const char *g = mpd_song_get_tag(song, MPD_TAG_GENRE, 0);
        const char *tr = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
        const char *disc = mpd_song_get_tag(song, MPD_TAG_DISC, 0);

        if (t && strlen(t) > 0) titleStr = t;

        std::string artistStr = a ? a : "";
        std::string albumStr = alb ? alb : "";
        if (!artistStr.empty() && !albumStr.empty()) {
          artistAlbumStr = artistStr + "  •  " + albumStr;
        } else if (!artistStr.empty()) {
          artistAlbumStr = artistStr;
        } else if (!albumStr.empty()) {
          artistAlbumStr = albumStr;
        }

        std::string dateStr = d ? d : "";
        std::string genreStr = g ? g : "";
        if (!dateStr.empty() && !genreStr.empty()) {
          yearGenreStr = dateStr + "  •  " + genreStr;
        } else if (!dateStr.empty()) {
          yearGenreStr = dateStr;
        } else if (!genreStr.empty()) {
          yearGenreStr = genreStr;
        }

        std::string trackStr = tr ? tr : "";
        std::string discStr = disc ? disc : "";
        if (!trackStr.empty() && !discStr.empty()) {
          trackDiscStr = "Track " + trackStr + "  •  Disc " + discStr;
        } else if (!trackStr.empty()) {
          trackDiscStr = "Track " + trackStr;
        } else if (!discStr.empty()) {
          trackDiscStr = "Disc " + discStr;
        }

        mpd_song_free(song);
      }

      struct mpd_status *status = mpd_run_status(conn);
      if (status) {
        const struct mpd_audio_format *af = mpd_status_get_audio_format(status);
        int kbitRate = mpd_status_get_kbit_rate(status);

        if (af) {
          float sampleRateKHz = af->sample_rate / 1000.0f;
          char fmtBuf[128];
          snprintf(fmtBuf, sizeof(fmtBuf), "%.1f kHz  •  %d-bit  •  %s",
                   sampleRateKHz, af->bits, (af->channels == 2 ? "Stereo" : "Mono"));
          qualityStr = fmtBuf;
        }
        if (kbitRate > 0) {
          if (!qualityStr.empty()) qualityStr += "  •  ";
          qualityStr += std::to_string(kbitRate) + " kbps";
        }

        mpd_status_free(status);
      }
    }

    if (m_titleText) m_titleText->rebuild()->text(std::string(titleStr))->commence();
    if (m_artistAlbumText) m_artistAlbumText->rebuild()->text(std::string(artistAlbumStr))->commence();
    if (m_yearGenreText) m_yearGenreText->rebuild()->text(std::string(yearGenreStr))->commence();
    if (m_qualityText) m_qualityText->rebuild()->text(std::string(qualityStr))->commence();
    if (m_trackDiscText) m_trackDiscText->rebuild()->text(std::string(trackDiscStr))->commence();

    // Check artwork updates
    if (m_lastSongUri != songUri || m_currentArtPath.empty() || m_currentArtPath == Utils::getDefaultArtworkPath()) {
      m_lastSongUri = songUri;
      std::string artPath = Utils::resolveTrackArtwork(conn, songUri);
      if (artPath.empty()) {
        artPath = Utils::getDefaultArtworkPath();
      }
      m_currentArtPath = artPath;
      if (m_bgImage && m_tabContentWrapper) {
        m_tabContentWrapper->removeChild(m_bgImage);
        m_bgImage = CImageBuilder::begin()
                        ->path(std::string(artPath))
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                            CDynamicSize::HT_SIZE_PERCENT,
                                            {1.0F, 1.0F}))
                        ->rounding(0)
                        ->fitMode(IMAGE_FIT_MODE_COVER)
                        ->sync(true)
                        ->commence();
        m_bgImage->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        m_bgImage->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
        m_tabContentWrapper->addChild(m_bgImage);
      }
    }

    if (m_tabContentWrapper) {
      m_tabContentWrapper->forceReposition();
    }
  });
}

} // namespace UI::Views

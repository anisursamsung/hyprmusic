#include "PlaybackBar.hpp"
#include "Utils/ArtworkUtils.hpp"

namespace UI::Components {

PlaybackBar::PlaybackBar(const PlaybackBarContext &ctx) : m_ctx(ctx) {}

void PlaybackBar::build(CSharedPointer<CColumnLayoutElement> parentColumn) {
  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  auto navCallback = m_ctx.onNavigationClick;

  auto triggerNav = [navCallback](Core::eViewMode targetMode) {
    if (navCallback) {
      navCallback(targetMode);
    }
  };

  // Outer Playback Section container
  auto playbackSection =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  // Left: Mini Artwork Card Section (20% width)
  auto miniArtworkCardSection =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->rounding(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {0.2F, 1.0F}))
          ->commence();
  miniArtworkCardSection->setMargin(8);

  MiniArtworkCardConfig cardCfg{
      .palette = palette,
      .fontFamily = fontFamily,
      .imagePath = Utils::getDefaultArtworkPath(),
      .title = "No currently playing songs",
      .subtitle = "",
      .onClick = [triggerNav]() { triggerNav(Core::eViewMode::VIEW_PLAYER); }};
  m_miniArtworkCard = std::make_unique<MiniArtworkCard>(cardCfg);
  miniArtworkCardSection->addChild(m_miniArtworkCard->build());

  // Right: Main Controls Column (80% width) containing Top, Middle, Bottom Subsections
  auto mainControlsSection =
      CColumnLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {0.8F, 1.0F}))
          ->commence();
  mainControlsSection->setMargin(6);

  // 1. Top Navigation Subsection
  TopSectionContext topCtx{
      .window = m_ctx.window,
      .backend = m_ctx.backend,
      .palette = palette,
      .fontFamily = fontFamily,
      .onNavigationClick = m_ctx.onNavigationClick};
  m_topSection = std::make_unique<TopSection>(topCtx);
  mainControlsSection->addChild(m_topSection->build());

  // 2. Middle Seekbar & Time Subsection
  MiddleSectionContext midCtx{
      .palette = palette,
      .fontFamily = fontFamily,
      .onSeek =
          [this](float pct) {
            m_ctx.runMpdCommand([pct](struct mpd_connection *conn) {
              struct mpd_status *status = mpd_run_status(conn);
              if (status) {
                unsigned total = mpd_status_get_total_time(status);
                if (total > 0) {
                  float seconds = pct * static_cast<float>(total);
                  mpd_run_seek_current(conn, seconds, false);
                }
                mpd_status_free(status);
              }
            });
          }};
  m_middleSection = std::make_unique<MiddleSection>(midCtx);
  mainControlsSection->addChild(m_middleSection->build());

  // 3. Bottom Transport & Volume Controls Subsection
  BottomSectionContext botCtx{
      .palette = palette,
      .fontFamily = fontFamily,
      .togglePlayPause = m_ctx.togglePlayPause,
      .prevTrack = m_ctx.prevTrack,
      .nextTrack = m_ctx.nextTrack,
      .onVolumeChange =
          [this](int vol) {
            m_ctx.runMpdCommand([vol](struct mpd_connection *conn) {
              mpd_run_set_volume(conn, vol);
            });
          },
      .onRepeatModeChange = m_ctx.onRepeatModeChange,
      .onRandomModeChange = m_ctx.onRandomModeChange,
      .onConsumeModeChange = m_ctx.onConsumeModeChange};
  m_bottomSection = std::make_unique<BottomSection>(botCtx);
  mainControlsSection->addChild(m_bottomSection->build());

  playbackSection->addChild(mainControlsSection);
  playbackSection->addChild(miniArtworkCardSection);
  parentColumn->addChild(playbackSection);
}

void PlaybackBar::updateTrackInfo(const std::string &title,
                                  const std::string &artist,
                                  bool hasActiveTrack, unsigned elapsed,
                                  unsigned total) {
  if (m_miniArtworkCard) {
    if (hasActiveTrack) {
      m_miniArtworkCard->updateInfo(title, artist);
    } else {
      m_miniArtworkCard->updateInfo("No currently playing songs", "");
    }
  }

  if (m_middleSection) {
    m_middleSection->updateProgress(elapsed, total, hasActiveTrack);
  }
}

void PlaybackBar::updateVolume(int currentVolume) {
  if (m_bottomSection) {
    m_bottomSection->updateVolume(currentVolume);
  }
}

void PlaybackBar::updatePlayPauseState(const std::string &stateText) {
  bool isPlaying = (stateText == "media-playback-pause" ||
                    stateText == "media-playback-pause-symbolic");

  if (m_topSection) {
    m_topSection->setPlaying(isPlaying);
  }
  if (m_bottomSection) {
    m_bottomSection->updatePlayPauseState(stateText);
  }
}

void PlaybackBar::updateRepeatState(bool isRepeat, bool isSingle) {
  if (m_bottomSection) {
    m_bottomSection->updateRepeatState(isRepeat, isSingle);
  }
}

void PlaybackBar::updateRandomState(bool isRandom) {
  if (m_bottomSection) {
    m_bottomSection->updateRandomState(isRandom);
  }
}

void PlaybackBar::updateConsumeState(bool isConsume) {
  if (m_bottomSection) {
    m_bottomSection->updateConsumeState(isConsume);
  }
}

void PlaybackBar::applyAlbumArt(const std::string &artPath) {
  if (m_miniArtworkCard) {
    m_miniArtworkCard->updateImage(artPath);
  }
}

void PlaybackBar::updateAlbumArt(const std::string &songUri) {
  if (songUri.empty()) {
    if (m_lastSongUri != "" ||
        m_currentArtPath != Utils::getDefaultArtworkPath()) {
      m_lastSongUri = "";
      m_currentArtPath = Utils::getDefaultArtworkPath();
      applyAlbumArt(m_currentArtPath);
    }
    return;
  }

  if (m_lastSongUri != songUri || m_currentArtPath == Utils::getDefaultArtworkPath()) {
    m_lastSongUri = songUri;

    m_ctx.runMpdCommand([this, songUri](struct mpd_connection *conn) {
      std::string artPath = Utils::getDefaultArtworkPath();
      if (conn) {
        artPath = Utils::resolveTrackArtwork(conn, songUri);
      }
      if (artPath.empty()) {
        artPath = Utils::getDefaultArtworkPath();
      }
      m_currentArtPath = artPath;
      applyAlbumArt(artPath);
    });
  }
}

void PlaybackBar::forceUpdateAlbumArt(const std::string &songUri) {
  m_lastSongUri = "__FORCE_REFRESH__";
  m_currentArtPath = "";
  updateAlbumArt(songUri);
}

void PlaybackBar::setActiveViewMode(Core::eViewMode mode) {
  if (m_topSection) {
    m_topSection->setActiveViewMode(mode);
  }
}

} // namespace UI::Components

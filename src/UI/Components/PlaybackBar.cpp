#include "PlaybackBar.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include "../../Utils/FormatUtils.hpp"
#include "IconProvider.hpp"
#include <algorithm>
#include <cmath>
#include <hyprtoolkit/system/Icons.hpp>

namespace UI::Components {

static std::string getVolumeFallbackEmoji(bool muted, int vol) {
  return IconProvider::getVolumeIcon(muted, vol);
}

PlaybackBar::PlaybackBar(const PlaybackBarContext &ctx) : m_ctx(ctx) {}

void PlaybackBar::build(CSharedPointer<CColumnLayoutElement> parentColumn) {
  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  auto navCallback = m_ctx.onNavigationClick;
  auto onPlayerNavClick = [navCallback](Input::eMouseButton button, bool down) {
    if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
      navCallback(Core::eViewMode::VIEW_PLAYER);
    }
  };
  auto onQueueNavClick = [navCallback](Input::eMouseButton button, bool down) {
    if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
      navCallback(Core::eViewMode::VIEW_QUEUE);
    }
  };
  auto onPlaylistNavClick = [navCallback](Input::eMouseButton button,
                                          bool down) {
    if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
      navCallback(Core::eViewMode::VIEW_PLAYLISTS);
    }
  };
  auto onDatabaseNavClick = [navCallback](Input::eMouseButton button,
                                          bool down) {
    if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
      navCallback(Core::eViewMode::VIEW_DATABASE);
    }
  };
  auto onYtdlpNavClick = [navCallback](Input::eMouseButton button, bool down) {
    if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
      navCallback(Core::eViewMode::VIEW_YTDLP);
    }
  };
  auto onVisNavClick = [navCallback](Input::eMouseButton button, bool down) {
    if (navCallback && button == Input::MOUSE_BUTTON_LEFT && !down) {
      navCallback(Core::eViewMode::VIEW_VISUALIZER);
    }
  };

  // Outer Playback Section container (20% of parentColumn)
  auto playbackSection =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.2F}))
          ->commence();

  auto leftLayout =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->rounding(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {0.2F, 1.0F}))
          ->commence();

  leftLayout->setMargin(8);

  CardViewConfig cardCfg{.palette = palette,
                         .fontFamily = fontFamily,
                         .imagePath = Utils::getDefaultArtworkPath(),
                         .title = "No currently playing songs",
                         .subtitle = "",
                         .text = "No currently playing songs",
                         .onClick = [onPlayerNavClick]() {
                           onPlayerNavClick(Input::MOUSE_BUTTON_LEFT, false);
                         }};
  m_cardView = std::make_unique<CardView>(cardCfg);
  leftLayout->addChild(m_cardView->build());

  auto rightLayout =
      CColumnLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {0.8F, 1.0F}))
          ->commence();

  playbackSection->addChild(leftLayout);
  playbackSection->addChild(rightLayout);

  // 1. Navigation bar section (100% width, 30% height of PlaybackSection)
  m_navigationBar =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.30F}))
          ->commence();

  // Horizontal Row Layout for navigation tabs
  auto navRow =
      CRowLayoutBuilder::begin()
          ->gap(5)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();

  m_navTabs.clear();

  auto addNavTab =
      [&](const std::string &fallbackLabel, Core::eViewMode mode,
          std::function<void(Input::eMouseButton, bool)> &&onClick) {
        auto res =
            createTabCell(fallbackLabel, 0.18F, std::move(onClick),
                          1.0f, CFontSize::HT_FONT_TEXT, true);
        navRow->addChild(res.container);
        m_navTabs.push_back({mode, res.container, res.textLabel, res.bottomIndicator});
      };

  // 1. Queue / List Icon
  addNavTab("Queue", Core::eViewMode::VIEW_QUEUE, std::move(onQueueNavClick));

  // 2. Database / Library Icon
  addNavTab("Database", Core::eViewMode::VIEW_DATABASE, std::move(onDatabaseNavClick));

  // 3. Playlist Icon
  addNavTab("Playlist", Core::eViewMode::VIEW_PLAYLISTS,
            std::move(onPlaylistNavClick));

  // 4. YT-DLP Icon
  addNavTab("YTDLP", Core::eViewMode::VIEW_YTDLP,
            std::move(onYtdlpNavClick));

  // 5. Mini Visualizer Container
  auto miniVisContainer =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->rounding(palette ? palette->m_vars.smallRounding : 5)
          ->borderThickness(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {0.18F, 1.0F}))
          ->commence();

  auto miniVisIndicator =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
          ->commence();
  miniVisIndicator->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  miniVisIndicator->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);
  miniVisContainer->addChild(miniVisIndicator);

  auto miniVisRow =
      CRowLayoutBuilder::begin()
          ->gap(3)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.60F}))
          ->commence();
  miniVisRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  miniVisRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  m_miniVisBars.clear();
  float defaultHeights[4] = {0.40f, 0.20f, 0.50f, 0.25f};
  for (int i = 0; i < 4; ++i) {
    auto bar = CRectangleBuilder::begin()
                   ->color([this, palette] {
                     if (m_activeViewMode == Core::eViewMode::VIEW_VISUALIZER) {
                       return palette ? palette->m_colors.text
                                      : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
                     }
                     return palette ? palette->m_colors.text
                                    : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
                   })
                   ->rounding(2)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {3.0F, defaultHeights[i]}))
                   ->commence();
    m_miniVisBars.push_back(bar);
    miniVisRow->addChild(bar);
  }
  miniVisContainer->addChild(miniVisRow);
  miniVisContainer->setReceivesMouse(true);
  miniVisContainer->setMouseButton(onVisNavClick);
  navRow->addChild(miniVisContainer);
  m_navTabs.push_back({Core::eViewMode::VIEW_VISUALIZER, miniVisContainer, nullptr, miniVisIndicator});

  updateNavTabStates();

  m_navigationBar->addChild(navRow);
  rightLayout->addChild(m_navigationBar);

  // 2. Seek bar section (30% of PlaybackSection)
  auto seekBarSection =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })

          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.30F}))
          ->commence();

  auto seekBarRow =
      CRowLayoutBuilder::begin()
          ->gap(12)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  seekBarRow->setMargin(10);

  m_timeText =
      CTextBuilder::begin()
          ->text(std::string("0:00 / 0:00"))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.8, 0.8, 0.8, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->align(HT_FONT_ALIGN_LEFT)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  m_timeText->setGrow(false);

  // Custom Seekbar Initialization
  CustomSeekBar::Context seekCtx{
      .window = m_ctx.window,
      .palette = palette,
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
          },
      .size = CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                           CDynamicSize::HT_SIZE_ABSOLUTE, {0.0F, 36.0F})};

  m_customSeekBar = std::make_unique<CustomSeekBar>(seekCtx);
  auto seekBarElem = m_customSeekBar->build();
  seekBarElem->setGrow(
      true); // Now stretches properly since width type is absolute

  seekBarRow->addChild(seekBarElem);
  seekBarRow->addChild(m_timeText);
  seekBarSection->addChild(seekBarRow);
  rightLayout->addChild(seekBarSection);

  // 3. Controls section (40% of PlaybackSection)
  auto iconFactory = m_ctx.backend ? m_ctx.backend->systemIcons() : nullptr;

  auto controlsSection =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })

          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.40F}))
          ->commence();

  auto controlsLayout =
      CRowLayoutBuilder::begin()
          ->gap(12)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  controlsLayout->setMargin(8);

  auto mainControlsRow =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  mainControlsRow->setGrow(true);

  auto addMediaControlCell =
      [&](const std::string &label, float fontScale,
          std::function<void(Input::eMouseButton, bool)> &&onClick) {
        auto res = createTabCell(label, 0.25F, std::move(onClick), fontScale);
        mainControlsRow->addChild(res.container);
        return res.textLabel;
      };

  // 1. Skip Backward
  addMediaControlCell(IconProvider::getIcon(IconType::PREV_TRACK), 1.5f,
                      [this](Input::eMouseButton button, bool down) {
                        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                          if (m_ctx.prevTrack)
                            m_ctx.prevTrack();
                        }
                      });

  // 2. Play / Pause
  m_pauseBtn = addMediaControlCell(
      IconProvider::getIcon(IconType::PLAY),
      1.9f, [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (m_ctx.togglePlayPause)
            m_ctx.togglePlayPause();
        }
      });

  // 3. Skip Forward
  addMediaControlCell(IconProvider::getIcon(IconType::NEXT_TRACK), 1.5f,
                      [this](Input::eMouseButton button, bool down) {
                        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                          if (m_ctx.nextTrack)
                            m_ctx.nextTrack();
                        }
                      });

  // 4. Volume Column
  {
    auto volCol =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.25F, 1.0F}))
            ->commence();

    auto volRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.85F, 1.0F}))
            ->commence();
    auto volIconBg =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->borderThickness(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {32.0F, 32.0F}))
            ->commence();

    m_volIcon =
        CTextBuilder::begin()
            ->text(getVolumeFallbackEmoji(m_isMuted,
                                          m_isMuted ? 0 : m_lastUnmutedVolume))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H1, 1.3f))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();

    m_volIcon->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    m_volIcon->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    volIconBg->addChild(m_volIcon);

    auto volToggle = [this](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down) {
        if (m_isMuted) {
          int targetVol = (m_lastUnmutedVolume > 0) ? m_lastUnmutedVolume : 50;
          m_isMuted = false;
          m_ctx.runMpdCommand([targetVol](struct mpd_connection *conn) {
            mpd_run_set_volume(conn, targetVol);
          });
          updateVolume(targetVol);
        } else {
          m_isMuted = true;
          m_ctx.runMpdCommand(
              [](struct mpd_connection *conn) { mpd_run_set_volume(conn, 0); });
          updateVolume(0);
        }
      }
    };

    volIconBg->setReceivesMouse(true);
    volIconBg->setMouseButton(std::move(volToggle));

    CustomSeekBar::Context volCtx{
        .window = m_ctx.window,
        .palette = palette,
        .onSeek =
            [this](float pct) {
              int vol = std::clamp(static_cast<int>(pct * 100.0f), 0, 100);
              if (vol > 0) {
                m_isMuted = false;
                m_lastUnmutedVolume = vol;
                updateVolumeIconState(false, vol);
              } else {
                m_isMuted = true;
                updateVolumeIconState(true, 0);
              }
              m_ctx.runMpdCommand([vol](struct mpd_connection *conn) {
                mpd_run_set_volume(conn, vol);
              });
            },
        .size = CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                             CDynamicSize::HT_SIZE_ABSOLUTE, {0.0F, 16.0F}),
        .rounding = 4};
    m_customVolumeBar = std::make_unique<CustomSeekBar>(volCtx);
    auto volElem = m_customVolumeBar->build();
    volElem->setGrow(true);

    volRow->addChild(volIconBg);
    volRow->addChild(volElem);
    volRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    volRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

    volCol->addChild(volRow);
    mainControlsRow->addChild(volCol);
  }

  controlsLayout->addChild(mainControlsRow);

  // Settings Icon Wrapper
  auto settingsWrapper =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT, {48.0F, 1.0F}))
          ->commence();

  auto settingsBg =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->borderThickness(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {32.0F, 32.0F}))
          ->commence();
  settingsBg->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  settingsBg->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  CSharedPointer<IElement> settingsIconBtn =
      CTextBuilder::begin()
          ->text(IconProvider::getIcon(IconType::SETTINGS))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0, 1.0, 1.0, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H1, 1.3f))
          ->align(HT_FONT_ALIGN_CENTER)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  settingsIconBtn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  settingsIconBtn->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  settingsBg->addChild(settingsIconBtn);
  settingsWrapper->addChild(settingsBg);

  auto settingsClick = [this](Input::eMouseButton button, bool down) {
    if (button == Input::MOUSE_BUTTON_LEFT && !down) {
      if (m_ctx.onNavigationClick)
        m_ctx.onNavigationClick(Core::eViewMode::VIEW_SETTINGS);
    }
  };

  settingsBg->setReceivesMouse(true);
  settingsBg->setMouseButton(std::move(settingsClick));

  controlsLayout->addChild(settingsWrapper);
  controlsSection->addChild(controlsLayout);
  rightLayout->addChild(controlsSection);
  parentColumn->addChild(playbackSection);
}

void PlaybackBar::updateTrackInfo(const std::string &title,
                                  const std::string &artist,
                                  bool hasActiveTrack, unsigned elapsed,
                                  unsigned total) {
  if (m_cardView) {
    if (hasActiveTrack) {
      m_cardView->updateInfo(title, artist);
    } else {
      m_cardView->updateInfo("No currently playing songs", "");
    }
  }
  if (m_timeText) {
    std::string timeStr = "0:00 / 0:00";
    if (hasActiveTrack && total > 0) {
      timeStr = Utils::formatTime(elapsed) + " / " + Utils::formatTime(total);
    }
    m_timeText->rebuild()->text(std::string(timeStr))->commence();
  }

  if (m_customSeekBar) {
    float progress = 0.0f;
    if (hasActiveTrack && total > 0) {
      progress = std::clamp(
          static_cast<float>(elapsed) / static_cast<float>(total), 0.0f, 1.0f);
    }
    m_customSeekBar->updateProgress(progress, hasActiveTrack);
  }
}

void PlaybackBar::updateVolumeIconState(bool muted, int vol) {
  if (!m_volIcon)
    return;

  int effectiveVol = vol >= 0 ? vol : (muted ? 0 : m_lastUnmutedVolume);
  auto textBtn = Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_volIcon);
  if (textBtn) {
    textBtn->rebuild()
        ->text(getVolumeFallbackEmoji(muted, effectiveVol))
        ->commence();
  }
}

void PlaybackBar::updateVolume(int currentVolume) {
  if (currentVolume >= 0) {
    if (currentVolume == 0) {
      m_isMuted = true;
      updateVolumeIconState(true, 0);
    } else {
      m_isMuted = false;
      m_lastUnmutedVolume = currentVolume;
      updateVolumeIconState(false, currentVolume);
    }
  }

  if (m_customVolumeBar) {
    float fraction = 0.0f;
    if (currentVolume >= 0) {
      fraction = static_cast<float>(currentVolume) / 100.0f;
    }
    m_customVolumeBar->updateProgress(fraction, true);
  }
}

void PlaybackBar::updatePlayPauseState(const std::string &stateText) {
  bool wasPlaying = m_isPlaying;
  m_isPlaying = (stateText == "media-playback-pause" ||
                 stateText == "media-playback-pause-symbolic");

  updateMiniVisBars();
  if (m_isPlaying && (!wasPlaying || !m_isMiniVisAnimating)) {
    scheduleMiniVisAnimation();
  }

  if (!m_pauseBtn)
    return;

  auto textBtn =
      Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_pauseBtn);
  if (textBtn) {
    textBtn->rebuild()
        ->text(IconProvider::getIcon(m_isPlaying ? IconType::PAUSE
                                                 : IconType::PLAY))
        ->commence();
  }
}

void PlaybackBar::applyAlbumArt(const std::string &artPath) {
  if (m_cardView) {
    m_cardView->updateImage(artPath);
  }
}

void PlaybackBar::updateAlbumArt(const std::string &songUri) {
  if (songUri.empty()) {
    m_lastSongUri = "";
    m_currentArtPath = Utils::getDefaultArtworkPath();
    applyAlbumArt(m_currentArtPath);
    return;
  }

  // Check if track changed OR if currently shown artwork is the default fallback
  if (m_lastSongUri != songUri || m_currentArtPath.empty() || m_currentArtPath == Utils::getDefaultArtworkPath()) {
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

void PlaybackBar::setActiveViewMode(Core::eViewMode mode) {
  m_activeViewMode = mode;
  updateNavTabStates();
}

void PlaybackBar::updateNavTabStates() {
  auto palette = m_ctx.palette;
  int smallRounding = palette ? palette->m_vars.smallRounding : 5;

  for (auto &tab : m_navTabs) {
    if (!tab.container)
      continue;

    bool isActive = (m_activeViewMode == tab.mode);

    tab.container->rebuild()
        ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
        ->rounding(smallRounding)
        ->borderThickness(0)
        ->commence();

    if (tab.textLabel) {
      tab.textLabel->rebuild()
          ->color([palette, isActive] {
            if (!isActive) {
              return palette ? palette->m_colors.text.mix(
                                   palette->m_colors.alternateBase, 0.4F)
                             : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
            }
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->commence();
    }

    if (tab.bottomIndicator) {
      tab.bottomIndicator->rebuild()
          ->color([palette, isActive] {
            if (isActive) {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
            }
            return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F);
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
          ->commence();
    }
  }
}

PlaybackBar::TabCellResult PlaybackBar::createTabCell(
    const std::string &fallbackLabel, float containerWidthPct,
    std::function<void(Input::eMouseButton, bool)> onClick, float fontScale,
    CFontSize::eSizingBase fontBase, bool withBorder) {

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  std::string boldFont = fontFamily.empty() ? "Sans Serif Bold" : (fontFamily + " Bold");

  auto builder = CRectangleBuilder::begin()
                     ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_PERCENT,
                                         {containerWidthPct, 1.0F}));

  CSharedPointer<CRectangleElement> bottomIndicator = nullptr;

  if (withBorder) {
    bottomIndicator =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
            ->commence();
    bottomIndicator->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bottomIndicator->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);
  }

  auto btnContainer = builder->commence();
  if (bottomIndicator) {
    btnContainer->addChild(bottomIndicator);
  }

  auto textLabelElem =
      CTextBuilder::begin()
          ->text(std::string(fallbackLabel))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->fontFamily(std::string(boldFont))
          ->fontSize(CFontSize(fontBase, fontScale))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  textLabelElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  textLabelElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  btnContainer->addChild(textLabelElem);

  btnContainer->setReceivesMouse(true);
  btnContainer->setMouseButton(std::move(onClick));

  return {btnContainer, textLabelElem, textLabelElem, bottomIndicator};
}

void PlaybackBar::updateMiniVisBars() {
  if (m_miniVisBars.size() < 4)
    return;

  auto palette = m_ctx.palette;
  if (m_isPlaying) {
    m_miniVisAnimPhase += 0.25f;
    float phases[4] = {0.0f, 1.2f, 2.4f, 3.6f};
    float multipliers[4] = {1.0f, 0.75f, 0.9f, 0.8f};

    for (size_t i = 0; i < 4; ++i) {
      float val =
          std::abs(std::sin(m_miniVisAnimPhase + phases[i])) * multipliers[i];
      float heightPct = 0.15f + val * 0.35f; // Height range: 15% to 50% of container height
      m_miniVisBars[i]
          ->rebuild()
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->rounding(2)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT, {3.0F, heightPct}))
          ->commence();
    }
  } else {
    // Static resting placeholder state when paused: unequal bars of various
    // height percentages
    float defaultHeights[4] = {0.40f, 0.20f, 0.50f, 0.25f};
    for (size_t i = 0; i < 4; ++i) {
      m_miniVisBars[i]
          ->rebuild()
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
          })
          ->rounding(2)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {3.0F, defaultHeights[i]}))
          ->commence();
    }
  }
}

void PlaybackBar::scheduleMiniVisAnimation() {
  if (!m_ctx.backend || m_isMiniVisAnimating)
    return;

  m_isMiniVisAnimating = true;

  m_ctx.backend->addTimer(
      std::chrono::milliseconds(100), // ~10 FPS ultra-light animation
      [this](CAtomicSharedPointer<CTimer>, void *) {
        updateMiniVisBars();

        if (m_isPlaying) {
          m_isMiniVisAnimating = false;
          scheduleMiniVisAnimation();
        } else {
          m_isMiniVisAnimating = false;
        }
      },
      nullptr);
}

} // namespace UI::Components

#include "PlaybackBar.hpp"
#include "../../Utils/FormatUtils.hpp"
#include <algorithm>
#include <hyprtoolkit/system/Icons.hpp>

namespace UI::Components {

PlaybackBar::PlaybackBar(const PlaybackBarContext &ctx) : m_ctx(ctx) {}

void PlaybackBar::build(CSharedPointer<CColumnLayoutElement> parentColumn) {
  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  // 1. Song info section
  auto songInfoSection =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 42.0F}))
          ->commence();

  auto nowPlayingContainer =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {0.5F, 1.0F}))
          ->commence();
  nowPlayingContainer->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  nowPlayingContainer->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
  nowPlayingContainer->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

  auto textWrapper =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  textWrapper->setGrow(true);

  m_nowPlayingText =
      CTextBuilder::begin()
          ->text(std::string("Track 1 - Unknown Artist"))
          ->color([palette] {
            return palette ? palette->m_colors.accent
                           : CHyprColor(0.2, 0.8, 0.4, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H2))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();

  textWrapper->addChild(m_nowPlayingText);
  nowPlayingContainer->addChild(textWrapper);

  songInfoSection->addChild(nowPlayingContainer);
  parentColumn->addChild(songInfoSection);

  // 2. Seek bar section
  auto seekBarSection =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 48.0F}))
          ->commence();

  auto seekBarRow =
      CRowLayoutBuilder::begin()
          ->gap(12)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  seekBarRow->setMargin(8);

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

  m_seekBar =
      CSliderBuilder::begin()
          ->min(0.0f)
          ->max(1.0f)
          ->val(0.0f)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 18.0F}))
          ->onChanged([this](CSharedPointer<CSliderElement>, float val) {
            if (m_isUpdatingSeekBar)
              return;
            m_ctx.runMpdCommand([val](struct mpd_connection *conn) {
              struct mpd_status *status = mpd_run_status(conn);
              if (status) {
                unsigned total = mpd_status_get_total_time(status);
                if (total > 0) {
                  float seconds = val * static_cast<float>(total);
                  mpd_run_seek_current(conn, seconds, false);
                }
                mpd_status_free(status);
              }
            });
          })
          ->commence();
  m_seekBar->setReceivesMouse(true);
  m_seekBar->setMouseButton([this](Input::eMouseButton button, bool down) {
    if (button == Input::MOUSE_BUTTON_LEFT && down) {
      auto cursorPos = m_ctx.window->cursorPos();
      auto sliderSize = m_seekBar->size();
      if (sliderSize.x > 0.0) {
        float pct = std::clamp(static_cast<float>(cursorPos.x / sliderSize.x),
                               0.0f, 1.0f);
        m_ctx.runMpdCommand([this, pct](struct mpd_connection *conn) {
          struct mpd_status *status = mpd_run_status(conn);
          if (status) {
            unsigned total = mpd_status_get_total_time(status);
            if (total > 0) {
              float seconds = pct * static_cast<float>(total);
              mpd_run_seek_current(conn, seconds, false);
              m_isUpdatingSeekBar = true;
              m_seekBar->rebuild()->val(pct)->commence();
              m_seekBar->setGrow(true);
              m_isUpdatingSeekBar = false;
            }
            mpd_status_free(status);
          }
        });
      }
    }
  });
  m_seekBar->setGrow(true);

  seekBarRow->addChild(m_timeText);
  seekBarRow->addChild(m_seekBar);
  seekBarSection->addChild(seekBarRow);
  parentColumn->addChild(seekBarSection);

  // 3. Controls section
  auto controlsSection =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.12F}))
          ->commence();

  auto controlsLayout =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();

  auto iconFactory = m_ctx.backend->systemIcons();

  auto addControlColumn =
      [&](const std::string &iconName, const std::string &fallbackLabel,
          std::function<void(Input::eMouseButton, bool)> &&onClick) {
        auto col = CRectangleBuilder::begin()
                       ->color([] { return CHyprColor(0, 0, 0, 0); })
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_PERCENT,
                                           {1.0F / 6.0F, 1.0F}))
                       ->commence();

        CSharedPointer<IElement> btn;
        CSharedPointer<ISystemIconDescription> iconDesc;
        if (iconFactory) {
          iconDesc = iconFactory->lookupIcon(iconName);
        }

        if (iconDesc) {
          btn = CImageBuilder::begin()
                    ->icon(iconDesc)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {0.6F, 0.6F}))
                    ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                    ->commence();
        } else {
          btn =
              CTextBuilder::begin()
                  ->text(std::string(fallbackLabel))
                  ->color([palette] {
                    return palette ? palette->m_colors.text
                                   : CHyprColor(1.0, 1.0, 1.0, 1.0);
                  })
                  ->fontFamily(std::string(fontFamily))
                  ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                  ->align(HT_FONT_ALIGN_CENTER)
                  ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                      CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                  ->interactable(true)
                  ->commence();
        }

        btn->setReceivesMouse(true);
        btn->setMouseButton(std::move(onClick));
        btn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        btn->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

        col->addChild(btn);
        controlsLayout->addChild(col);
        return btn;
      };

  // Volume column
  {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F / 6.0F, 1.0F}))
                   ->commence();

    auto volRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {0.85F, 1.0F}))
            ->commence();
    volRow->setMargin(6);

    CSharedPointer<ISystemIconDescription> iconDesc;
    if (iconFactory) {
      iconDesc = iconFactory->lookupIcon(m_isMuted ? "audio-volume-muted"
                                                   : "audio-volume-high");
    }

    if (iconDesc) {
      m_volIcon = CImageBuilder::begin()
                      ->icon(iconDesc)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                          CDynamicSize::HT_SIZE_ABSOLUTE,
                                          {18.0F, 18.0F}))
                      ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                      ->commence();
    } else {
      m_volIcon =
          CTextBuilder::begin()
              ->text(std::string(m_isMuted ? "🔇" : "🔊"))
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(1.0, 1.0, 1.0, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->align(HT_FONT_ALIGN_CENTER)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->interactable(true)
              ->commence();
    }
    m_volIcon->setGrow(false);

    m_volIcon->setReceivesMouse(true);
    m_volIcon->setMouseButton([this](Input::eMouseButton button, bool down) {
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
    });

    m_volumeSlider =
        CSliderBuilder::begin()
            ->min(0.0f)
            ->max(1.0f)
            ->val(1.0f)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
            ->onChanged([this](CSharedPointer<CSliderElement>, float val) {
              if (m_isUpdatingVolumeSlider)
                return;
              int vol = std::clamp(static_cast<int>(val * 100.0f), 0, 100);
              if (vol > 0) {
                m_isMuted = false;
                m_lastUnmutedVolume = vol;
                updateVolumeIconState(false);
              } else {
                m_isMuted = true;
                updateVolumeIconState(true);
              }
              m_ctx.runMpdCommand([vol](struct mpd_connection *conn) {
                mpd_run_set_volume(conn, vol);
              });
            })
            ->commence();
    m_volumeSlider->setReceivesMouse(true);
    m_volumeSlider->setGrow(true);

    m_volumeSlider->setMouseButton([this](Input::eMouseButton button,
                                          bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && down) {
        auto cursorPos = m_ctx.window->cursorPos();
        auto sliderSize = m_volumeSlider->size();
        if (sliderSize.x > 0.0) {
          float pct = std::clamp(static_cast<float>(cursorPos.x / sliderSize.x),
                                 0.0f, 1.0f);
          int vol = std::clamp(static_cast<int>(pct * 100.0f), 0, 100);
          if (vol > 0) {
            m_isMuted = false;
            m_lastUnmutedVolume = vol;
            updateVolumeIconState(false);
          } else {
            m_isMuted = true;
            updateVolumeIconState(true);
          }
          m_ctx.runMpdCommand([this, pct, vol](struct mpd_connection *conn) {
            mpd_run_set_volume(conn, vol);
            m_isUpdatingVolumeSlider = true;
            m_volumeSlider->rebuild()
                ->min(0.0f)
                ->max(1.0f)
                ->val(pct)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 16.0F}))
                ->commence();
            m_volumeSlider->setGrow(true);
            m_isUpdatingVolumeSlider = false;
          });
        }
      }
    });

    volRow->addChild(m_volIcon);
    volRow->addChild(m_volumeSlider);

    volRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    volRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

    col->addChild(volRow);
    controlsLayout->addChild(col);
  }

  m_repeatBtn = addControlColumn(
      "media-playlist-repeat", "🔁",
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          bool targetRepeat = !m_isRepeat;
          m_ctx.runMpdCommand([targetRepeat](struct mpd_connection *conn) {
            mpd_run_repeat(conn, targetRepeat);
          });
          updatePlaybackModes(targetRepeat, m_isRandom);
        }
      });

  addControlColumn("media-skip-backward", "⏮",
                   [this](Input::eMouseButton button, bool down) {
                     if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                       if (m_ctx.prevTrack)
                         m_ctx.prevTrack();
                     }
                   });

  m_pauseBtn =
      addControlColumn("media-playback-start", "▶",
                       [this](Input::eMouseButton button, bool down) {
                         if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                           if (m_ctx.togglePlayPause)
                             m_ctx.togglePlayPause();
                         }
                       });

  addControlColumn("media-skip-forward", "⏭",
                   [this](Input::eMouseButton button, bool down) {
                     if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                       if (m_ctx.nextTrack)
                         m_ctx.nextTrack();
                     }
                   });

  m_randomBtn = addControlColumn(
      "media-playlist-shuffle", "🔀",
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          bool targetRandom = !m_isRandom;
          m_ctx.runMpdCommand([targetRandom](struct mpd_connection *conn) {
            mpd_run_random(conn, targetRandom);
          });
          updatePlaybackModes(m_isRepeat, targetRandom);
        }
      });

  controlsSection->addChild(controlsLayout);
  parentColumn->addChild(controlsSection);
}

void PlaybackBar::updateTrackInfo(const std::string &trackText,
                                  bool hasActiveTrack, unsigned elapsed,
                                  unsigned total) {
  if (m_nowPlayingText) {
    std::string textToDisplay =
        hasActiveTrack ? trackText : "No currently playing songs";
    m_nowPlayingText->rebuild()->text(std::string(textToDisplay))->commence();
  }

  if (m_timeText) {
    std::string timeStr = "0:00 / 0:00";
    if (hasActiveTrack && total > 0) {
      timeStr = Utils::formatTime(elapsed) + " / " + Utils::formatTime(total);
    }
    m_timeText->rebuild()->text(std::string(timeStr))->commence();
  }

  if (m_seekBar && !m_seekBar->sliding()) {
    m_isUpdatingSeekBar = true;
    float progress = 0.0f;
    if (hasActiveTrack && total > 0) {
      progress = std::clamp(
          static_cast<float>(elapsed) / static_cast<float>(total), 0.0f, 1.0f);
    }
    m_seekBar->rebuild()
        ->min(0.0f)
        ->max(1.0f)
        ->val(progress)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                            CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
        ->commence();
    m_seekBar->setGrow(true);
    m_isUpdatingSeekBar = false;
  }
}

void PlaybackBar::updateVolumeIconState(bool muted) {
  if (!m_volIcon)
    return;

  auto imgBtn = Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_volIcon);
  if (imgBtn) {
    auto iconFactory = m_ctx.backend->systemIcons();
    auto iconDesc = iconFactory
                        ? iconFactory->lookupIcon(muted ? "audio-volume-muted"
                                                        : "audio-volume-high")
                        : nullptr;
    if (iconDesc) {
      imgBtn->rebuild()->icon(iconDesc)->commence();
    }
  } else {
    auto textBtn =
        Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_volIcon);
    if (textBtn) {
      std::string iconChar = muted ? "🔇" : "🔊";
      textBtn->rebuild()->text(std::move(iconChar))->commence();
    }
  }
}

void PlaybackBar::updateVolume(int currentVolume) {
  if (currentVolume >= 0) {
    if (currentVolume == 0) {
      m_isMuted = true;
      updateVolumeIconState(true);
    } else {
      m_isMuted = false;
      m_lastUnmutedVolume = currentVolume;
      updateVolumeIconState(false);
    }
  }

  if (m_volumeSlider && !m_volumeSlider->sliding()) {
    m_isUpdatingVolumeSlider = true;
    float fraction = 0.0f;
    if (currentVolume >= 0) {
      fraction = static_cast<float>(currentVolume) / 100.0f;
    }
    m_volumeSlider->rebuild()
        ->min(0.0f)
        ->max(1.0f)
        ->val(fraction)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                            CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 16.0F}))
        ->commence();
    m_volumeSlider->setGrow(true);
    m_isUpdatingVolumeSlider = false;
  }
}

void PlaybackBar::updatePlayPauseState(const std::string &stateText) {
  if (!m_pauseBtn)
    return;

  auto imgBtn =
      Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_pauseBtn);
  if (imgBtn) {
    auto iconFactory = m_ctx.backend->systemIcons();
    auto iconDesc =
        iconFactory
            ? iconFactory->lookupIcon(stateText == "⏸" ? "media-playback-pause"
                                                       : "media-playback-start")
            : nullptr;
    if (iconDesc) {
      imgBtn->rebuild()->icon(iconDesc)->commence();
    }
  } else {
    auto textBtn =
        Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_pauseBtn);
    if (textBtn) {
      textBtn->rebuild()->text(std::string(stateText))->commence();
    }
  }
}

void PlaybackBar::updatePlaybackModes(bool repeat, bool random) {
  m_isRepeat = repeat;
  m_isRandom = random;

  auto palette = m_ctx.palette;
  auto iconFactory = m_ctx.backend->systemIcons();

  if (m_repeatBtn) {
    auto imgBtn =
        Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_repeatBtn);
    if (imgBtn && iconFactory) {
      std::string iconName =
          repeat ? "media-playlist-repeat-symbolic" : "media-playlist-repeat";
      auto iconDesc = iconFactory->lookupIcon(iconName);
      if (!iconDesc && repeat)
        iconDesc = iconFactory->lookupIcon("media-playlist-repeat-song");
      if (!iconDesc && repeat)
        iconDesc = iconFactory->lookupIcon("media-playlist-repeat");
      if (iconDesc)
        imgBtn->rebuild()->icon(iconDesc)->commence();
    } else {
      auto textBtn =
          Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_repeatBtn);
      if (textBtn) {
        std::string label = repeat ? "🔂" : "🔁";
        textBtn->rebuild()
            ->text(std::move(label))
            ->color([palette, repeat] {
              if (repeat) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return palette ? palette->m_colors.text
                             : CHyprColor(0.8, 0.8, 0.8, 1.0);
            })
            ->commence();
      }
    }
  }

  if (m_randomBtn) {
    auto imgBtn =
        Hyprutils::Memory::dynamicPointerCast<CImageElement>(m_randomBtn);
    if (imgBtn && iconFactory) {
      std::string iconName =
          random ? "media-playlist-shuffle-symbolic" : "media-playlist-shuffle";
      auto iconDesc = iconFactory->lookupIcon(iconName);
      if (!iconDesc && random)
        iconDesc = iconFactory->lookupIcon("media-playlist-shuffle");
      if (iconDesc)
        imgBtn->rebuild()->icon(iconDesc)->commence();
    } else {
      auto textBtn =
          Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_randomBtn);
      if (textBtn) {
        std::string label = random ? "🔀✨" : "🔀";
        textBtn->rebuild()
            ->text(std::move(label))
            ->color([palette, random] {
              if (random) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return palette ? palette->m_colors.text
                             : CHyprColor(0.8, 0.8, 0.8, 1.0);
            })
            ->commence();
      }
    }
  }
}

} // namespace UI::Components

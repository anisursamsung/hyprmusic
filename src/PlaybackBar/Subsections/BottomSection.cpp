#include "BottomSection.hpp"
#include "Utils/IconProvider.hpp"
#include <algorithm>

namespace UI::Components {

static std::string getVolumeFallbackEmoji(bool muted, int vol) {
  return IconProvider::getVolumeIcon(muted, vol);
}

BottomSection::BottomSection(const BottomSectionContext &ctx) : m_ctx(ctx) {}

CSharedPointer<CRectangleElement> BottomSection::build() {
  auto palette = m_ctx.palette;

  auto controlsSection =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto controlsLayout =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  controlsLayout->setMargin(6);

  // 1. Volume Control (Left Cell - 22% width)
  {
    auto volCol =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {0.22F, 40.0F}))
            ->commence();
    volCol->setGrow(false);

    auto volRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    auto volIconBg =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->borderThickness(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {40.0F, 40.0F}))
            ->commence();

    m_volIcon =
        CTextBuilder::begin()
            ->text(getVolumeFallbackEmoji(m_isMuted,
                                          m_isMuted ? 0 : m_lastUnmutedVolume))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(IconProvider::getCustomFontFamily())
            ->fontSize(CFontSize(CFontSize::HT_FONT_ABSOLUTE, 20.0f))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    m_volIcon->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    m_volIcon->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    volIconBg->addChild(m_volIcon);

    auto volToggle = [this](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down) {
        if (m_isMuted) {
          int targetVol = (m_lastUnmutedVolume > 0) ? m_lastUnmutedVolume : 50;
          m_isMuted = false;
          if (m_ctx.onVolumeChange)
            m_ctx.onVolumeChange(targetVol);
          updateVolume(targetVol);
        } else {
          m_isMuted = true;
          if (m_ctx.onVolumeChange)
            m_ctx.onVolumeChange(0);
          updateVolume(0);
        }
      }
    };

    volIconBg->setReceivesMouse(true);
    volIconBg->setMouseButton(std::move(volToggle));

    CustomSeekBar::Context volCtx{
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
              if (m_ctx.onVolumeChange) {
                m_ctx.onVolumeChange(vol);
              }
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
    controlsLayout->addChild(volCol);
  }

  // 2. Shuffle / Random Button (Cell 2/7 - 12% width, 16.0f font scale)
  auto shuffleRes = createTabCell(
      IconProvider::getIcon(IconType::SHUFFLE), 0.12F,
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          m_isRandom = !m_isRandom;
          renderShuffleButtonState();
          if (m_ctx.onRandomModeChange) {
            m_ctx.onRandomModeChange(m_isRandom);
          }
        }
      },
      16.0f, CFontSize::HT_FONT_ABSOLUTE);
  m_shuffleContainer = shuffleRes.container;
  m_shuffleBtn = shuffleRes.textLabel;
  controlsLayout->addChild(shuffleRes.container);
  renderShuffleButtonState();

  // 3. Skip Backward (Cell 3/7 - 14% width)
  auto prevRes = createTabCell(
      IconProvider::getIcon(IconType::PREV_TRACK), 0.14F,
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (m_ctx.prevTrack)
            m_ctx.prevTrack();
        }
      },
      20.0f, CFontSize::HT_FONT_ABSOLUTE);
  controlsLayout->addChild(prevRes.container);

  // 4. Play / Pause (Cell 4/7 - 16% width)
  auto pauseRes = createTabCell(
      IconProvider::getIcon(IconType::PLAY), 0.16F,
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (m_ctx.togglePlayPause)
            m_ctx.togglePlayPause();
        }
      },
      22.0f, CFontSize::HT_FONT_ABSOLUTE);
  m_pauseBtn = pauseRes.textLabel;
  controlsLayout->addChild(pauseRes.container);

  // 5. Skip Forward (Cell 5/7 - 14% width)
  auto nextRes = createTabCell(
      IconProvider::getIcon(IconType::NEXT_TRACK), 0.14F,
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (m_ctx.nextTrack)
            m_ctx.nextTrack();
        }
      },
      20.0f, CFontSize::HT_FONT_ABSOLUTE);
  controlsLayout->addChild(nextRes.container);

  // 6. Repeat Mode Cycle (Cell 6/7 - 11% width, 16.0f font scale)
  auto repeatRes = createTabCell(
      IconProvider::getIcon(IconType::REPEAT_OFF), 0.11F,
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          eRepeatMode nextMode = eRepeatMode::REPEAT_OFF;
          if (m_repeatMode == eRepeatMode::REPEAT_OFF) {
            nextMode = eRepeatMode::REPEAT_ALL;
          } else if (m_repeatMode == eRepeatMode::REPEAT_ALL) {
            nextMode = eRepeatMode::REPEAT_ONCE;
          } else {
            nextMode = eRepeatMode::REPEAT_OFF;
          }

          m_repeatMode = nextMode;
          renderRepeatButtonState();

          if (m_ctx.onRepeatModeChange) {
            bool rep = (nextMode == eRepeatMode::REPEAT_ALL ||
                        nextMode == eRepeatMode::REPEAT_ONCE);
            bool sng = (nextMode == eRepeatMode::REPEAT_ONCE);
            m_ctx.onRepeatModeChange(rep, sng);
          }
        }
      },
      16.0f, CFontSize::HT_FONT_ABSOLUTE);
  m_repeatBtn = repeatRes.textLabel;
  controlsLayout->addChild(repeatRes.container);
  renderRepeatButtonState();

  // 7. Consume Mode Toggle (Cell 7/7 - 11% width, 16.0f font scale)
  auto consumeRes = createTabCell(
      IconProvider::getIcon(IconType::CONSUME), 0.11F,
      [this](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          m_isConsume = !m_isConsume;
          renderConsumeButtonState();
          if (m_ctx.onConsumeModeChange) {
            m_ctx.onConsumeModeChange(m_isConsume);
          }
        }
      },
      16.0f, CFontSize::HT_FONT_ABSOLUTE);
  m_consumeContainer = consumeRes.container;
  m_consumeBtn = consumeRes.textLabel;
  controlsLayout->addChild(consumeRes.container);
  renderConsumeButtonState();

  controlsSection->addChild(controlsLayout);
  return controlsSection;
}

void BottomSection::renderShuffleButtonState() {
  if (!m_shuffleContainer || !m_shuffleBtn)
    return;

  auto palette = m_ctx.palette;
  int smallRounding = palette ? palette->m_vars.smallRounding : 5;

  m_shuffleContainer->rebuild()
      ->color([palette, this] {
        if (m_isRandom) {
          return palette ? palette->m_colors.alternateBase
                         : CHyprColor(0.25F, 0.25F, 0.30F, 1.0F);
        }
        return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F);
      })
      ->rounding(smallRounding)
      ->borderThickness(0)
      ->commence();

  m_shuffleBtn->rebuild()
      ->color([palette, this] {
        if (m_isRandom) {
          return palette ? palette->m_colors.brightText
                         : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
        }
        return palette ? palette->m_colors.text.mix(
                             palette->m_colors.alternateBase, 0.4F)
                       : CHyprColor(0.55F, 0.55F, 0.55F, 1.0F);
      })
      ->commence();
}

void BottomSection::renderConsumeButtonState() {
  if (!m_consumeContainer || !m_consumeBtn)
    return;

  auto palette = m_ctx.palette;
  int smallRounding = palette ? palette->m_vars.smallRounding : 5;

  m_consumeContainer->rebuild()
      ->color([palette, this] {
        if (m_isConsume) {
          return palette ? palette->m_colors.alternateBase
                         : CHyprColor(0.25F, 0.25F, 0.30F, 1.0F);
        }
        return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F);
      })
      ->rounding(smallRounding)
      ->borderThickness(0)
      ->commence();

  m_consumeBtn->rebuild()
      ->color([palette, this] {
        if (m_isConsume) {
          return palette ? palette->m_colors.brightText
                         : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
        }
        return palette ? palette->m_colors.text.mix(
                             palette->m_colors.alternateBase, 0.4F)
                       : CHyprColor(0.55F, 0.55F, 0.55F, 1.0F);
      })
      ->commence();
}

void BottomSection::renderRepeatButtonState() {
  if (!m_repeatBtn)
    return;

  auto palette = m_ctx.palette;
  IconType targetIcon = IconType::REPEAT_OFF;
  bool isActive = false;

  if (m_repeatMode == eRepeatMode::REPEAT_ALL) {
    targetIcon = IconType::REPEAT_ALL;
    isActive = true;
  } else if (m_repeatMode == eRepeatMode::REPEAT_ONCE) {
    targetIcon = IconType::REPEAT_ONCE;
    isActive = true;
  } else {
    targetIcon = IconType::REPEAT_OFF;
    isActive = false;
  }

  m_repeatBtn->rebuild()
      ->text(IconProvider::getIcon(targetIcon))
      ->color([palette, isActive] {
        if (isActive) {
          return palette ? palette->m_colors.brightText
                         : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
        }
        return palette ? palette->m_colors.text.mix(
                             palette->m_colors.alternateBase, 0.4F)
                       : CHyprColor(0.55F, 0.55F, 0.55F, 1.0F);
      })
      ->fontFamily(IconProvider::getCustomFontFamily())
      ->commence();
}

void BottomSection::updateRandomState(bool isRandom) {
  if (m_isRandom != isRandom) {
    m_isRandom = isRandom;
    renderShuffleButtonState();
  }
}

void BottomSection::updateConsumeState(bool isConsume) {
  if (m_isConsume != isConsume) {
    m_isConsume = isConsume;
    renderConsumeButtonState();
  }
}

void BottomSection::updateRepeatState(bool isRepeat, bool isSingle) {
  eRepeatMode mode = eRepeatMode::REPEAT_OFF;
  if (isRepeat && isSingle) {
    mode = eRepeatMode::REPEAT_ONCE;
  } else if (isRepeat && !isSingle) {
    mode = eRepeatMode::REPEAT_ALL;
  } else {
    mode = eRepeatMode::REPEAT_OFF;
  }

  if (m_repeatMode != mode) {
    m_repeatMode = mode;
    renderRepeatButtonState();
  }
}

void BottomSection::updateVolumeIconState(bool muted, int vol) {
  if (!m_volIcon)
    return;

  int effectiveVol = vol >= 0 ? vol : (muted ? 0 : m_lastUnmutedVolume);
  auto textBtn = Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_volIcon);
  if (textBtn) {
    textBtn->rebuild()
        ->text(getVolumeFallbackEmoji(muted, effectiveVol))
        ->fontFamily(IconProvider::getCustomFontFamily())
        ->commence();
  }
}

void BottomSection::updateVolume(int currentVolume) {
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

void BottomSection::updatePlayPauseState(const std::string &stateText) {
  m_isPlaying = (stateText == "media-playback-pause" ||
                 stateText == "media-playback-pause-symbolic");

  if (!m_pauseBtn)
    return;

  auto textBtn =
      Hyprutils::Memory::dynamicPointerCast<CTextElement>(m_pauseBtn);
  if (textBtn) {
    textBtn->rebuild()
        ->text(IconProvider::getIcon(m_isPlaying ? IconType::PAUSE
                                                 : IconType::PLAY))
        ->fontFamily(IconProvider::getCustomFontFamily())
        ->commence();
  }
}

BottomSection::TabCellResult BottomSection::createTabCell(
    const std::string &fallbackLabel, float containerWidthPct,
    std::function<void(Input::eMouseButton, bool)> onClick, float fontScale,
    CFontSize::eSizingBase fontBase) {

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  std::string boldFont =
      fontFamily.empty() ? "Sans Serif Bold" : (fontFamily + " Bold");

  auto btnContainer =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE,
                              {containerWidthPct, 40.0F}))
          ->commence();

  std::string targetFont = boldFont;
  if (IconProvider::isCustomFontIcon(fallbackLabel)) {
    targetFont = IconProvider::getCustomFontFamily();
  }

  auto textLabelElem =
      CTextBuilder::begin()
          ->text(std::string(fallbackLabel))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->fontFamily(std::string(targetFont))
          ->fontSize(CFontSize(fontBase, fontScale))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto clickCb = onClick;
  textLabelElem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  textLabelElem->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
  textLabelElem->setReceivesMouse(true);
  textLabelElem->setMouseButton(
      [clickCb](Input::eMouseButton button, bool down) {
        if (clickCb)
          clickCb(button, down);
      });
  btnContainer->addChild(textLabelElem);

  btnContainer->setReceivesMouse(true);
  btnContainer->setMouseButton(
      [clickCb](Input::eMouseButton button, bool down) {
        if (clickCb)
          clickCb(button, down);
      });

  return {btnContainer, textLabelElem, textLabelElem, nullptr};
}

} // namespace UI::Components

#pragma once

#include "Common/CustomSeekBar.hpp"
#include <functional>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <memory>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

enum class eRepeatMode {
  REPEAT_OFF,
  REPEAT_ALL,
  REPEAT_ONCE
};

struct BottomSectionContext {
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void()> togglePlayPause;
  std::function<void()> prevTrack;
  std::function<void()> nextTrack;
  std::function<void(int vol)> onVolumeChange;
  std::function<void(bool repeat, bool single)> onRepeatModeChange;
  std::function<void(bool random)> onRandomModeChange;
  std::function<void(bool consume)> onConsumeModeChange;
};

class BottomSection {
public:
  explicit BottomSection(const BottomSectionContext &ctx);

  CSharedPointer<CRectangleElement> build();
  void updateVolume(int currentVolume);
  void updatePlayPauseState(const std::string &stateText);
  void updateRepeatState(bool isRepeat, bool isSingle);
  void updateRandomState(bool isRandom);
  void updateConsumeState(bool isConsume);

private:
  struct TabCellResult {
    CSharedPointer<CRectangleElement> container;
    CSharedPointer<CTextElement> textLabel;
    CSharedPointer<IElement> iconElem;
    CSharedPointer<CRectangleElement> bottomIndicator;
  };

  TabCellResult
  createTabCell(const std::string &fallbackLabel, float containerWidthPct,
                std::function<void(Input::eMouseButton, bool)> onClick,
                float fontScale = 20.0f,
                CFontSize::eSizingBase fontBase = CFontSize::HT_FONT_ABSOLUTE);

  void updateVolumeIconState(bool muted, int vol = -1);
  void renderRepeatButtonState();
  void renderShuffleButtonState();
  void renderConsumeButtonState();

  BottomSectionContext m_ctx;
  std::unique_ptr<CustomSeekBar> m_customVolumeBar;
  CSharedPointer<IElement> m_pauseBtn;
  CSharedPointer<IElement> m_volIcon;
  CSharedPointer<CTextElement> m_repeatBtn;
  CSharedPointer<CRectangleElement> m_shuffleContainer;
  CSharedPointer<CTextElement> m_shuffleBtn;
  CSharedPointer<CRectangleElement> m_consumeContainer;
  CSharedPointer<CTextElement> m_consumeBtn;

  bool m_isMuted = false;
  int m_lastUnmutedVolume = 70;
  bool m_isPlaying = false;
  eRepeatMode m_repeatMode = eRepeatMode::REPEAT_OFF;
  bool m_isRandom = false;
  bool m_isConsume = false;
};

} // namespace UI::Components

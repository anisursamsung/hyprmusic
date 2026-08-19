#pragma once

#include "../../Core/ViewMode.hpp"
#include "CardView.hpp"
#include "CustomSeekBar.hpp"
#include <functional>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <memory>
#include <mpd/client.h>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlaybackBarContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void(const std::function<void(struct ::mpd_connection *)> &)>
      runMpdCommand;
  std::function<void()> togglePlayPause;
  std::function<void()> prevTrack;
  std::function<void()> nextTrack;
  std::function<void(Core::eViewMode targetMode)> onNavigationClick;
};

class PlaybackBar {
public:
  explicit PlaybackBar(const PlaybackBarContext &ctx);
  void build(CSharedPointer<CColumnLayoutElement> parentColumn);
  void updateTrackInfo(const std::string &title, const std::string &artist,
                       bool hasActiveTrack, unsigned elapsed, unsigned total);
  void updateVolume(int currentVolume);
  void updatePlayPauseState(const std::string &stateText);
  void updateAlbumArt(const std::string &songUri);
  void forceUpdateAlbumArt(const std::string &songUri);
  void setActiveViewMode(Core::eViewMode mode);

private:
  struct TabCellResult {
    CSharedPointer<CRectangleElement> container;
    CSharedPointer<CTextElement> textLabel;
    CSharedPointer<IElement> iconElem;
    CSharedPointer<CRectangleElement> bottomIndicator;
  };

  struct NavTab {
    Core::eViewMode mode;
    CSharedPointer<CRectangleElement> container;
    CSharedPointer<CTextElement> textLabel;
    CSharedPointer<CRectangleElement> bottomIndicator;
  };

  TabCellResult
  createTabCell(const std::string &fallbackLabel, float containerWidthPct,
                std::function<void(Input::eMouseButton, bool)> onClick,
                float fontScale = 20.0f,
                CFontSize::eSizingBase fontBase = CFontSize::HT_FONT_ABSOLUTE,
                bool withBorder = false);

  void updateNavTabStates();

  Core::eViewMode m_activeViewMode = Core::eViewMode::VIEW_PLAYER;
  std::vector<NavTab> m_navTabs;
  CSharedPointer<CRectangleElement> m_navigationBar;
  PlaybackBarContext m_ctx;
  std::unique_ptr<CardView> m_cardView;
  CSharedPointer<CTextElement> m_timeText;
  CSharedPointer<CSliderElement> m_seekBar;
  bool m_isUpdatingSeekBar = false;
  std::unique_ptr<CustomSeekBar> m_customVolumeBar;
  CSharedPointer<IElement> m_pauseBtn;
  CSharedPointer<IElement> m_volIcon;
  bool m_isMuted = false;
  int m_lastUnmutedVolume = 70;
  void updateVolumeIconState(bool muted, int vol = -1);
  void applyAlbumArt(const std::string &artPath);
  std::unique_ptr<CustomSeekBar> m_customSeekBar;

  // Set to a dummy value so the first update ALWAYS runs
  std::string m_lastSongUri = "__INIT__";
  std::string m_currentArtPath = "";
  bool m_isPlaying = false;

  // Mini visualizer members
  std::vector<CSharedPointer<CRectangleElement>> m_miniVisBars;
  bool m_isMiniVisAnimating = false;
  float m_miniVisAnimPhase = 0.0f;
  void scheduleMiniVisAnimation();
  void updateMiniVisBars();
};

} // namespace UI::Components

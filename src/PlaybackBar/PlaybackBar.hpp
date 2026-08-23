#pragma once

#include "Core/ViewMode.hpp"
#include "MiniArt/MiniArtworkCard.hpp"
#include "Subsections/TopSection.hpp"
#include "Subsections/MiddleSection.hpp"
#include "Subsections/BottomSection.hpp"
#include <functional>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
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
  std::function<void(bool repeat, bool single)> onRepeatModeChange;
  std::function<void(bool random)> onRandomModeChange;
  std::function<void(bool consume)> onConsumeModeChange;
};

class PlaybackBar {
public:
  explicit PlaybackBar(const PlaybackBarContext &ctx);

  void build(CSharedPointer<CColumnLayoutElement> parentColumn);
  void updateTrackInfo(const std::string &title, const std::string &artist,
                       bool hasActiveTrack, unsigned elapsed, unsigned total);
  void updateVolume(int currentVolume);
  void updatePlayPauseState(const std::string &stateText);
  void updateRepeatState(bool isRepeat, bool isSingle);
  void updateRandomState(bool isRandom);
  void updateConsumeState(bool isConsume);
  void updateAlbumArt(const std::string &songUri);
  void forceUpdateAlbumArt(const std::string &songUri);
  void setActiveViewMode(Core::eViewMode mode);

private:
  void applyAlbumArt(const std::string &artPath);

  PlaybackBarContext m_ctx;
  std::unique_ptr<MiniArtworkCard> m_miniArtworkCard;
  std::unique_ptr<TopSection> m_topSection;
  std::unique_ptr<MiddleSection> m_middleSection;
  std::unique_ptr<BottomSection> m_bottomSection;

  std::string m_lastSongUri = "__INIT__";
  std::string m_currentArtPath = "";
};

} // namespace UI::Components

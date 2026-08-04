#pragma once

#include "CenteredTextLabel.hpp"
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>
#include "../../Core/ViewMode.hpp"

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlaybackBarContext {
    CSharedPointer<IWindow> window;
    CSharedPointer<IBackend> backend;
    CSharedPointer<CPalette> palette;
    std::string fontFamily;
    std::function<void(const std::function<void(struct ::mpd_connection *)> &)> runMpdCommand;
    std::function<void()> togglePlayPause;
    std::function<void()> prevTrack;
    std::function<void()> nextTrack;
    std::function<void(Core::eViewMode targetMode)> onNavigationClick;
};

class PlaybackBar {
public:
    explicit PlaybackBar(const PlaybackBarContext &ctx);
    void build(CSharedPointer<CColumnLayoutElement> parentColumn);
    void updateTrackInfo(const std::string &trackText, bool hasActiveTrack, unsigned elapsed, unsigned total);
    void updateVolume(int currentVolume);
    void updatePlayPauseState(const std::string &stateText);
    void updateAlbumArt(const std::string &songUri);

private:
    PlaybackBarContext m_ctx;
 std::unique_ptr<CenteredTextLabel> m_nowPlayingLabel;
    CSharedPointer<CTextElement> m_timeText;
    CSharedPointer<CSliderElement> m_seekBar;
    bool m_isUpdatingSeekBar = false;
    CSharedPointer<CSliderElement> m_volumeSlider;
    bool m_isUpdatingVolumeSlider = false;
    CSharedPointer<IElement> m_pauseBtn;
    CSharedPointer<IElement> m_volIcon;
    bool m_isMuted = false;
    int m_lastUnmutedVolume = 70;
    void updateVolumeIconState(bool muted);
  
  CSharedPointer<CRectangleElement> m_artContainer;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CImageElement> m_albumArt;
    
    // Set to a dummy value so the first update ALWAYS runs
    std::string m_lastSongUri = "__INIT__"; 
    bool m_isPlaying = false;
  
};

} // namespace UI::Components

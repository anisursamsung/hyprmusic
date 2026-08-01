#pragma once
#include "../Components/Visualizer.hpp" 
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlayerViewContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
};

class PlayerView {
public:
  explicit PlayerView(const PlayerViewContext &ctx);

  void rebuildUI(CSharedPointer<CRectangleElement> wrapper, struct mpd_connection *conn);
  void updateTrackInfo(const std::string &trackText, bool hasActiveTrack, unsigned elapsed, unsigned total, const std::string &songUri);





  private:
  void toggleVisualizer();
  
  PlayerViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  
  CSharedPointer<IElement> m_coverCardElementBackground;
  CSharedPointer<IElement> m_vignetteOverlay;
  CSharedPointer<IElement> m_coverCardElement;
  CSharedPointer<IElement> m_menuBtn;
  
  // Store the active visualizer instance
  std::shared_ptr<UI::Components::Visualizer> m_visualizer; 
  
  std::string m_lastSongUri;
  bool m_showVisualizer = false;
};

} // namespace UI::Views

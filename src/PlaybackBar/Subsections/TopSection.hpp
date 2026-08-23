#pragma once

#include "Core/ViewMode.hpp"
#include <functional>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <memory>
#include <string>
#include <vector>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct TopSectionContext {
  CSharedPointer<IWindow> window;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void(Core::eViewMode targetMode)> onNavigationClick;
};

class TopSection {
public:
  explicit TopSection(const TopSectionContext &ctx);

  CSharedPointer<CRectangleElement> build();
  void setActiveViewMode(Core::eViewMode mode);
  void setPlaying(bool isPlaying);

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
  void updateMiniVisBars();
  void scheduleMiniVisAnimation();

  TopSectionContext m_ctx;
  Core::eViewMode m_activeViewMode = Core::eViewMode::VIEW_PLAYER;
  std::vector<NavTab> m_navTabs;
  CSharedPointer<CRectangleElement> m_navigationBar;

  // Mini visualizer live tab
  std::vector<CSharedPointer<CRectangleElement>> m_miniVisBars;
  bool m_isPlaying = false;
  bool m_isMiniVisAnimating = false;
  float m_miniVisAnimPhase = 0.0f;
};

} // namespace UI::Components

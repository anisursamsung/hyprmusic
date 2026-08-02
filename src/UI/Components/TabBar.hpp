#pragma once
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../../Core/ViewMode.hpp"

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class TabBar {
public:
  TabBar(CSharedPointer<CPalette> palette, const std::string &fontFamily,
         std::function<void(Core::eViewMode)> onSwitchMode);
  CSharedPointer<CRectangleElement> build();
  void updateActiveTab(Core::eViewMode activeMode);

private:
  void populateTabs(); // Helper function to rebuild tabs

  CSharedPointer<CPalette> m_palette;
  std::string m_fontFamily;
  std::function<void(Core::eViewMode)> m_onSwitchMode;
  
  CSharedPointer<CRowLayoutElement> m_tabsRow; // Stores the row container
  Core::eViewMode m_activeMode = Core::eViewMode::VIEW_PLAYER;
};

} // namespace UI::Components

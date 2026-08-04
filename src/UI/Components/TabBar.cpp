#include "TabBar.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>

namespace UI::Components {

TabBar::TabBar(CSharedPointer<CPalette> palette, const std::string &fontFamily,
               std::function<void(Core::eViewMode)> onSwitchMode)
    : m_palette(palette), m_fontFamily(fontFamily), m_onSwitchMode(onSwitchMode) {}

CSharedPointer<CRectangleElement> TabBar::build() {
  auto palette = m_palette;

  // Outer container for the sidebar
  auto tabsSection = CRectangleBuilder::begin()
      ->color([palette] {
        return palette ? palette->m_colors.background
                       : CHyprColor(0.15, 0.15, 0.15, 1.0);
      })
      ->rounding(0)
      ->size(CDynamicSize(
          CDynamicSize::HT_SIZE_PERCENT,
          CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
      ->commence();

  // Vertical Scroll Area for sidebar items
  auto scrollArea = CScrollAreaBuilder::begin()
      ->scrollX(false)
      ->scrollY(true)
      ->blockUserScroll(false)
      ->size(CDynamicSize(
          CDynamicSize::HT_SIZE_PERCENT,
          CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
      ->commence();

  // Column layout for vertical stacking of tabs
  m_tabsRow = CColumnLayoutBuilder::begin()
      ->gap(12)
      ->size(CDynamicSize(
          CDynamicSize::HT_SIZE_PERCENT,
          CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
      ->commence();

  m_tabsRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_tabsRow->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);

  populateTabs();
  scrollArea->addChild(m_tabsRow);
  tabsSection->addChild(scrollArea);
  return tabsSection;
}

void TabBar::populateTabs() {
  if (!m_tabsRow)
    return;
  m_tabsRow->clearChildren();

  auto palette = m_palette;
  std::string fontFamily = m_fontFamily;

  struct TabDef {
    std::string label;
    Core::eViewMode mode;
  };

  std::vector<TabDef> tabs = {
      {"Player", Core::eViewMode::VIEW_PLAYER},
      {"Queue", Core::eViewMode::VIEW_QUEUE},
      {"Database", Core::eViewMode::VIEW_DATABASE},
      {"Playlists", Core::eViewMode::VIEW_PLAYLISTS},
      {"YT DLP", Core::eViewMode::VIEW_YTDLP},
      {"Visualizer", Core::eViewMode::VIEW_VISUALIZER},
      {"Settings", Core::eViewMode::VIEW_SETTINGS},
      {"Help", Core::eViewMode::VIEW_HELP},
  };

  for (const auto &tab : tabs) {
    auto mode = tab.mode;
    bool isActive = (mode == m_activeMode);

    // Reverting back to pill-style container with ghost border and active accent border/color
    auto pill = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->borderThickness(1)
        ->borderColor([palette, isActive] {
          if (isActive)
            return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
          return palette ? palette->m_colors.alternateBase : CHyprColor(0.18, 0.18, 0.18, 1.0);
        })
        ->rounding(16)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_ABSOLUTE, {0.9F, 40.0F}))
        ->commence();

    pill->setReceivesMouse(true);
    pill->setGrow(false);
    pill->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    pill->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);

    pill->setMouseButton([this, mode](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down) {
        if (m_onSwitchMode)
          m_onSwitchMode(mode);
      }
    });

    auto textLayout = CRowLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                            CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();

    auto padLeft = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                            CDynamicSize::HT_SIZE_PERCENT, {16.0F, 1.0F}))
        ->commence();
    padLeft->setGrow(false);
    textLayout->addChild(padLeft);

    auto tabBtnText = CTextBuilder::begin()
        ->text(std::string(tab.label))
        ->color([palette, isActive] {
          if (isActive)
            return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
          return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
        })
        ->fontFamily(std::string(fontFamily))
        ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
        ->align(HT_FONT_ALIGN_CENTER)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                            CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();

    textLayout->addChild(tabBtnText);

    auto padRight = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                            CDynamicSize::HT_SIZE_PERCENT, {16.0F, 1.0F}))
        ->commence();
    padRight->setGrow(false);
    textLayout->addChild(padRight);

    pill->addChild(textLayout);
    m_tabsRow->addChild(pill);
  }
}

void TabBar::updateActiveTab(Core::eViewMode activeMode) {
  m_activeMode = activeMode;
  populateTabs();
  if (m_tabsRow)
    m_tabsRow->forceReposition();
}

} // namespace UI::Components

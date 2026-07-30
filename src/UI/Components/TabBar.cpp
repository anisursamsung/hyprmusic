#include "TabBar.hpp"

namespace UI::Components {

TabBar::TabBar(CSharedPointer<CPalette> palette, const std::string &fontFamily,
               std::function<void(Core::eViewMode)> onSwitchMode)
    : m_palette(palette), m_fontFamily(fontFamily), m_onSwitchMode(onSwitchMode) {}

CSharedPointer<CRectangleElement> TabBar::build() {
  auto palette = m_palette;
  std::string fontFamily = m_fontFamily;

  auto tabsSection =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.background
                           : CHyprColor(0.15, 0.15, 0.15, 1.0);
          })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 42.0F}))
          ->commence();
  tabsSection->setGrow(false);

  auto tabsRow =
      CRowLayoutBuilder::begin()
          ->gap(15)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  tabsRow->setMargin(12);

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
      {"Settings", Core::eViewMode::VIEW_SETTINGS},
      {"Help", Core::eViewMode::VIEW_HELP},
  };

  for (const auto &tab : tabs) {
    auto mode = tab.mode;
    auto tabBtn =
        CTextBuilder::begin()
            ->text(std::string(tab.label))
            ->color([palette] {
              return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->interactable(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    tabBtn->setReceivesMouse(true);
    tabBtn->setMouseButton([this, mode](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down) {
        if (m_onSwitchMode)
          m_onSwitchMode(mode);
      }
    });

    auto tabLine =
        CRectangleBuilder::begin()
            ->color([palette] { return CHyprColor(0, 0, 0, 0); })
            ->rounding(1)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
            ->commence();

    m_tabLines[mode] = tabLine;

    auto col =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    col->addChild(tabBtn);
    col->addChild(tabLine);
    tabsRow->addChild(col);
  }

  tabsSection->addChild(tabsRow);
  return tabsSection;
}

void TabBar::updateActiveTab(Core::eViewMode activeMode) {
  auto palette = m_palette;
  for (auto &[mode, line] : m_tabLines) {
    if (!line)
      continue;
    bool isActive = (mode == activeMode);
    line->rebuild()
        ->color([palette, isActive] {
          return isActive ? (palette ? palette->m_colors.accent
                                     : CHyprColor(0.2, 0.8, 0.4, 1.0))
                          : CHyprColor(0, 0, 0, 0);
        })
        ->commence();
  }
}

} // namespace UI::Components

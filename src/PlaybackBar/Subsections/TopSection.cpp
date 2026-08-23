#include "TopSection.hpp"
#include "Utils/IconProvider.hpp"
#include <algorithm>
#include <cmath>

namespace UI::Components {

TopSection::TopSection(const TopSectionContext &ctx) : m_ctx(ctx) {}

CSharedPointer<CRectangleElement> TopSection::build() {
  auto palette = m_ctx.palette;
  auto navCallback = m_ctx.onNavigationClick;

  auto triggerNav = [navCallback](Core::eViewMode targetMode) {
    if (navCallback) {
      navCallback(targetMode);
    }
  };

  m_navigationBar =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto navRow =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  navRow->setMargin(6);

  m_navTabs.clear();

  const float cellWidth = 1.0F / 6.0F;

  auto addNavTab = [&](IconType iconType, Core::eViewMode mode) {
    auto res = createTabCell(
        IconProvider::getIcon(iconType), cellWidth,
        [triggerNav, mode](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            triggerNav(mode);
          }
        },
        20.0f, CFontSize::HT_FONT_ABSOLUTE, true);
    navRow->addChild(res.container);
    m_navTabs.push_back(
        {mode, res.container, res.textLabel, res.bottomIndicator});
  };

  // 1. Queue Tab
  addNavTab(IconType::NAV_QUEUE, Core::eViewMode::VIEW_QUEUE);

  // 2. Database Tab
  addNavTab(IconType::NAV_DATABASE, Core::eViewMode::VIEW_DATABASE);

  // 3. Playlists Tab
  addNavTab(IconType::NAV_PLAYLIST, Core::eViewMode::VIEW_PLAYLISTS);

  // 4. Online / Search Tab
  addNavTab(IconType::NAV_YTDLP, Core::eViewMode::VIEW_YTDLP);

  // 5. Visualizer Tab (Live animated equalizer cell)
  {
    auto miniVisWrapper =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {cellWidth, 40.0F}))
            ->commence();

    auto miniVisContainer =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
            ->rounding(palette ? palette->m_vars.smallRounding : 5)
            ->borderThickness(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {40.0F, 40.0F}))
            ->commence();
    miniVisContainer->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    miniVisContainer->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

    auto miniVisIndicator =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
            ->commence();
    miniVisIndicator->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    miniVisIndicator->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);
    miniVisContainer->addChild(miniVisIndicator);

    auto miniVisRow =
        CRowLayoutBuilder::begin()
            ->gap(3)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
            ->commence();
    miniVisRow->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    miniVisRow->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

    m_miniVisBars.clear();
    float defaultHeights[4] = {0.40f, 0.20f, 0.50f, 0.25f};
    for (int i = 0; i < 4; ++i) {
      auto bar = CRectangleBuilder::begin()
                     ->color([this, palette] {
                       if (m_activeViewMode == Core::eViewMode::VIEW_VISUALIZER) {
                         return palette ? palette->m_colors.text
                                        : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
                       }
                       return palette ? palette->m_colors.text
                                      : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
                     })
                     ->rounding(2)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                         CDynamicSize::HT_SIZE_PERCENT,
                                         {3.0F, defaultHeights[i]}))
                     ->commence();
      m_miniVisBars.push_back(bar);
      miniVisRow->addChild(bar);
    }
    miniVisContainer->addChild(miniVisRow);
    miniVisContainer->setReceivesMouse(true);
    miniVisContainer->setMouseButton(
        [triggerNav](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            triggerNav(Core::eViewMode::VIEW_VISUALIZER);
          }
        });

    miniVisWrapper->addChild(miniVisContainer);
    m_navTabs.push_back({Core::eViewMode::VIEW_VISUALIZER, miniVisContainer,
                         nullptr, miniVisIndicator});
    navRow->addChild(miniVisWrapper);
  }

  // 6. Settings Tab
  addNavTab(IconType::SETTINGS, Core::eViewMode::VIEW_SETTINGS);

  updateNavTabStates();

  m_navigationBar->addChild(navRow);
  return m_navigationBar;
}

void TopSection::setActiveViewMode(Core::eViewMode mode) {
  m_activeViewMode = mode;
  updateNavTabStates();
}

void TopSection::setPlaying(bool isPlaying) {
  bool wasPlaying = m_isPlaying;
  m_isPlaying = isPlaying;
  updateMiniVisBars();
  if (m_isPlaying && (!wasPlaying || !m_isMiniVisAnimating)) {
    scheduleMiniVisAnimation();
  }
}

void TopSection::updateNavTabStates() {
  auto palette = m_ctx.palette;
  int smallRounding = palette ? palette->m_vars.smallRounding : 5;

  for (auto &tab : m_navTabs) {
    if (!tab.container)
      continue;

    bool isActive = (m_activeViewMode == tab.mode);

    tab.container->rebuild()
        ->color([palette, isActive] {
          if (isActive) {
            return palette ? palette->m_colors.alternateBase
                           : CHyprColor(0.25F, 0.25F, 0.30F, 1.0F);
          }
          return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F);
        })
        ->rounding(smallRounding)
        ->borderThickness(0)
        ->commence();

    if (tab.textLabel) {
      tab.textLabel->rebuild()
          ->color([palette, isActive] {
            if (!isActive) {
              return palette ? palette->m_colors.text.mix(
                                   palette->m_colors.alternateBase, 0.4F)
                             : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
            }
            return palette ? palette->m_colors.brightText
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->commence();
    }

    if (tab.bottomIndicator) {
      tab.bottomIndicator->rebuild()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 0.0F}))
          ->commence();
    }
  }
}

TopSection::TabCellResult TopSection::createTabCell(
    const std::string &fallbackLabel, float containerWidthPct,
    std::function<void(Input::eMouseButton, bool)> onClick, float fontScale,
    CFontSize::eSizingBase fontBase, bool withBorder) {

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  std::string boldFont =
      fontFamily.empty() ? "Sans Serif Bold" : (fontFamily + " Bold");

  auto builder = CRectangleBuilder::begin()
                     ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_ABSOLUTE,
                                         {containerWidthPct, 40.0F}));

  CSharedPointer<CRectangleElement> bottomIndicator = nullptr;

  if (withBorder) {
    bottomIndicator =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
            ->commence();
    bottomIndicator->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bottomIndicator->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);
    bottomIndicator->setReceivesMouse(false);
  }

  auto btnContainer = builder->commence();
  if (bottomIndicator) {
    btnContainer->addChild(bottomIndicator);
  }

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

  return {btnContainer, textLabelElem, textLabelElem, bottomIndicator};
}

void TopSection::updateMiniVisBars() {
  if (m_miniVisBars.size() < 4)
    return;

  auto palette = m_ctx.palette;
  if (m_isPlaying) {
    m_miniVisAnimPhase += 0.25f;
    float phases[4] = {0.0f, 1.2f, 2.4f, 3.6f};
    float multipliers[4] = {1.0f, 0.75f, 0.9f, 0.8f};

    for (size_t i = 0; i < 4; ++i) {
      float val =
          std::abs(std::sin(m_miniVisAnimPhase + phases[i])) * multipliers[i];
      float heightPct = 0.15f + val * 0.35f;
      m_miniVisBars[i]
          ->rebuild()
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
          })
          ->rounding(2)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT, {3.0F, heightPct}))
          ->commence();
    }
  } else {
    float defaultHeights[4] = {0.40f, 0.20f, 0.50f, 0.25f};
    for (size_t i = 0; i < 4; ++i) {
      m_miniVisBars[i]
          ->rebuild()
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.6F, 0.6F, 0.6F, 1.0F);
          })
          ->rounding(2)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {3.0F, defaultHeights[i]}))
          ->commence();
    }
  }
}

void TopSection::scheduleMiniVisAnimation() {
  if (!m_ctx.backend || m_isMiniVisAnimating)
    return;

  m_isMiniVisAnimating = true;

  m_ctx.backend->addTimer(
      std::chrono::milliseconds(100),
      [this](CAtomicSharedPointer<CTimer>, void *) {
        updateMiniVisBars();

        if (m_isPlaying) {
          m_isMiniVisAnimating = false;
          scheduleMiniVisAnimation();
        } else {
          m_isMiniVisAnimating = false;
        }
      },
      nullptr);
}

} // namespace UI::Components

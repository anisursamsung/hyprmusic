#include "ActionMenuDialog.hpp"
#include "../Components/UIFactory.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <algorithm>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

static CSharedPointer<IWindow> s_activeMenuWindow = nullptr;

void showActionMenuDialog(const ActionMenuContext &ctx) {
  if (!ctx.parentWindow || ctx.options.empty())
    return;

  if (s_activeMenuWindow) {
    s_activeMenuWindow->close();
    s_activeMenuWindow = nullptr;
  }

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;

  auto windowSize = ctx.parentWindow->pixelSize();

  Hyprutils::Math::Vector2D anchorPos = ctx.anchorPos;
  Hyprutils::Math::Vector2D anchorSize = ctx.anchorSize;

  if (anchorPos.x < 0 || anchorPos.y < 0) {
    anchorPos = ctx.parentWindow->cursorPos();
    anchorSize = {0.0, 0.0};
  }

  // Calculate dynamic content width & height based on text lengths & option count
  size_t maxLen = ctx.title.empty() ? 0 : ctx.title.length();
  for (const auto &opt : ctx.options) {
    maxLen = std::max(maxLen, opt.length());
  }
  double popupWidth = std::clamp(maxLen * 9.5 + 36.0, 160.0, std::min(450.0, windowSize.x - 20.0));

  double headerH = ctx.title.empty() ? 0.0 : 32.0;
  double contentHeight = headerH + (ctx.options.size() * 38.0) + 16.0;
  double popupHeight = std::min(std::max(60.0, contentHeight), std::min(480.0, windowSize.y - 20.0));

  // Vertical Gravity Calculation (Default: Below anchor)
  double posY = anchorPos.y + anchorSize.y + 4.0;
  if (posY + popupHeight > windowSize.y) {
    // Reverse direction: Place above anchor if space allows
    double posAbove = anchorPos.y - popupHeight - 4.0;
    if (posAbove >= 0) {
      posY = posAbove;
    } else {
      posY = std::max(0.0, windowSize.y - popupHeight);
    }
  }

  // Horizontal Gravity Calculation (Default: Align to anchor/cursor)
  double posX = anchorPos.x;
  if (posX + popupWidth > windowSize.x) {
    // Reverse direction: Align to left side of anchor/cursor
    double posLeft = (anchorSize.x > 0) ? (anchorPos.x + anchorSize.x - popupWidth) : (anchorPos.x - popupWidth);
    if (posLeft >= 0) {
      posX = posLeft;
    } else {
      posX = std::max(0.0, windowSize.x - popupWidth);
    }
  }

  // Safety clamping within window boundaries
  posX = std::clamp(posX, 0.0, std::max(0.0, windowSize.x - popupWidth));
  posY = std::clamp(posY, 0.0, std::max(0.0, windowSize.y - popupHeight));

  auto popupWindow =
      CWindowBuilder::begin()
          ->type(HT_WINDOW_POPUP)
          ->parent(ctx.parentWindow)
          ->pos(Hyprutils::Math::Vector2D(posX, posY))
          ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
          ->commence();

  if (!popupWindow)
    return;

  s_activeMenuWindow = popupWindow;

  auto closeMenuWindow = [popupWindow] {
    if (popupWindow)
      popupWindow->close();
    if (s_activeMenuWindow == popupWindow)
      s_activeMenuWindow = nullptr;
  };

  auto root = Components::UIFactory::createCard(palette, 8);
  popupWindow->m_rootElement = root;

  auto cardLayout =
      CColumnLayoutBuilder::begin()
          ->gap(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  cardLayout->setMargin(8);

  if (!ctx.title.empty()) {
    auto headerText = Components::UIFactory::createHeader(ctx.title, palette, fontFamily);
    cardLayout->addChild(headerText);
  }

  auto scrollArea =
      CScrollAreaBuilder::begin()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->scrollY(true)
          ->commence();

  auto listLayout =
      CColumnLayoutBuilder::begin()
          ->gap(6)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  listLayout->setMargin(0);

  for (size_t i = 0; i < ctx.options.size(); ++i) {
    std::string optName = ctx.options[i];
    size_t optIdx = i;

    auto optBtn =
        CButtonBuilder::begin()
            ->label(std::string(optName))
            ->alignText(HT_FONT_ALIGN_LEFT)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([ctx, optIdx, optName, closeMenuWindow](CSharedPointer<CButtonElement>) {
              if (ctx.backend) {
                ctx.backend->addTimer(
                    std::chrono::milliseconds(20),
                    [closeMenuWindow, ctx, optIdx, optName](CAtomicSharedPointer<CTimer>, void *) {
                      closeMenuWindow();
                      if (ctx.onSelect)
                        ctx.onSelect(optIdx, optName);
                    },
                    nullptr);
              } else {
                closeMenuWindow();
                if (ctx.onSelect)
                  ctx.onSelect(optIdx, optName);
              }
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
            ->commence();
    optBtn->setGrow(false);

    listLayout->addChild(optBtn);
  }

  scrollArea->addChild(listLayout);
  cardLayout->addChild(scrollArea);

  root->addChild(cardLayout);
  popupWindow->open();
}

} // namespace UI::Dialogs

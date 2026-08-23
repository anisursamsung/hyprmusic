#include "ActionMenuDialog.hpp"
#include "BaseDialog.hpp"
#include "Utils/IconProvider.hpp"
#include "Utils/UIFactory.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <algorithm>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

CSharedPointer<IWindow> showActionMenuDialog(const ActionMenuContext &ctx) {
  if (!ctx.parentWindow || ctx.options.empty())
    return nullptr;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;
  auto windowSize = ctx.parentWindow->pixelSize();

  // Calculate dynamic content width & height based on text lengths & option count
  size_t maxLen = ctx.title.empty() ? 0 : ctx.title.length();
  for (const auto &opt : ctx.options) {
    maxLen = std::max(maxLen, opt.length());
  }
  double popupWidth = std::clamp(maxLen * 9.5 + 36.0, 160.0, std::min(450.0, windowSize.x - 20.0));

  double headerH = ctx.title.empty() ? 0.0 : 32.0;
  double contentHeight = headerH + (ctx.options.size() * 38.0) + 16.0;
  double popupHeight = std::min(std::max(60.0, contentHeight), std::min(480.0, windowSize.y - 20.0));

  auto res = BaseDialog::createAnchoredPopup(
      ctx.parentWindow,
      ctx.anchorPos,
      ctx.anchorSize,
      Hyprutils::Math::Vector2D(popupWidth, popupHeight),
      palette,
      8.0f, 8, 8);

  if (!res.window)
    return nullptr;

  if (ctx.onWindowCreated) {
    ctx.onWindowCreated(res.window);
  }

  auto popupWindow = res.window;
  auto closeMenuWindow = [popupWindow] {
    if (popupWindow)
      popupWindow->close();
  };

  if (!ctx.title.empty()) {
    auto headerText = Components::UIFactory::createHeader(ctx.title, palette, fontFamily);
    res.contentLayout->addChild(headerText);
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

    std::string targetFont = fontFamily;
    if (Components::IconProvider::isCustomFontIcon(optName)) {
      targetFont = Components::IconProvider::getCustomFontFamily();
    }

    auto optBtn =
        CButtonBuilder::begin()
            ->label(std::string(optName))
            ->alignText(HT_FONT_ALIGN_LEFT)
            ->fontFamily(std::string(targetFont))
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
  res.contentLayout->addChild(scrollArea);

  popupWindow->open();
  return popupWindow;
}

} // namespace UI::Dialogs

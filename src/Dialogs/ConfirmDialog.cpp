#include "ConfirmDialog.hpp"
#include "BaseDialog.hpp"
#include "Utils/UIFactory.hpp"
#include <hyprtoolkit/element/Text.hpp>

namespace UI::Dialogs {

CSharedPointer<IWindow> showConfirmDialog(const ConfirmDialogContext &ctx) {
  if (!ctx.parentWindow)
    return nullptr;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;

  auto res = BaseDialog::createCenteredModal(
      ctx.parentWindow,
      Hyprutils::Math::Vector2D(ctx.width, ctx.height),
      palette, 10.0f, 12, 15);

  if (!res.window)
    return nullptr;

  if (ctx.onWindowCreated) {
    ctx.onWindowCreated(res.window);
  }

  auto headerText = BaseDialog::createHeader(ctx.title, ctx.icon, palette, fontFamily);
  res.contentLayout->addChild(headerText);

  auto msgLabel =
      CTextBuilder::begin()
          ->text(std::string(ctx.message))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.85, 0.85, 0.85, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->align(HT_FONT_ALIGN_LEFT)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  res.contentLayout->addChild(msgLabel);

  auto windowPtr = res.window;
  auto btnRow = BaseDialog::createButtonRow(
      ctx.cancelLabel,
      [windowPtr, ctx] {
        if (ctx.onCancel)
          ctx.onCancel();
        if (windowPtr)
          windowPtr->close();
      },
      ctx.confirmLabel,
      [windowPtr, ctx] {
        if (ctx.onConfirm)
          ctx.onConfirm();
        if (windowPtr)
          windowPtr->close();
      },
      palette, fontFamily, !ctx.isDestructive, 20);

  res.contentLayout->addChild(btnRow);
  res.window->open();

  return res.window;
}

} // namespace UI::Dialogs

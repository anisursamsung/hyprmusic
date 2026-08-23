#include "PromptDialog.hpp"
#include "BaseDialog.hpp"
#include "Utils/UIFactory.hpp"
#include <memory>

namespace UI::Dialogs {

CSharedPointer<IWindow> showPromptDialog(const PromptDialogContext &ctx) {
  if (!ctx.parentWindow)
    return nullptr;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;
  auto inputText = std::make_shared<std::string>(ctx.initialText);

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

  auto textInput = Components::UIFactory::createSearchInput(
      ctx.placeholder, ctx.initialText,
      [inputText](const std::string &text) { *inputText = text; },
      palette, fontFamily);
  res.contentLayout->addChild(textInput);

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
      [windowPtr, ctx, inputText] {
        std::string val = *inputText;
        if (ctx.onConfirm)
          ctx.onConfirm(val);
        if (windowPtr)
          windowPtr->close();
      },
      palette, fontFamily, ctx.confirmIsAccent, 20);

  res.contentLayout->addChild(btnRow);
  res.window->open();

  return res.window;
}

} // namespace UI::Dialogs

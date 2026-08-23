#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <functional>
#include <string>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PromptDialogContext {
  std::string title = "Prompt";
  std::string icon = "";
  std::string placeholder = "Enter value...";
  std::string initialText = "";
  std::string cancelLabel = "Cancel";
  std::string confirmLabel = "Ok";
  bool confirmIsAccent = true;
  double width = 340.0;
  double height = 170.0;

  CSharedPointer<IWindow> parentWindow = nullptr;
  CSharedPointer<IBackend> backend = nullptr;
  CSharedPointer<CPalette> palette = nullptr;
  std::string fontFamily = "";

  std::function<void(const std::string &)> onConfirm = nullptr;
  std::function<void()> onCancel = nullptr;
  std::function<void(CSharedPointer<IWindow>)> onWindowCreated = nullptr;
};

CSharedPointer<IWindow> showPromptDialog(const PromptDialogContext &ctx);

} // namespace UI::Dialogs

#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <functional>
#include <string>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct ConfirmDialogContext {
  std::string title = "Confirm";
  std::string icon = "";
  std::string message = "Are you sure?";
  std::string cancelLabel = "Cancel";
  std::string confirmLabel = "Confirm";
  bool isDestructive = false;
  double width = 340.0;
  double height = 150.0;

  CSharedPointer<IWindow> parentWindow = nullptr;
  CSharedPointer<IBackend> backend = nullptr;
  CSharedPointer<CPalette> palette = nullptr;
  std::string fontFamily = "";

  std::function<void()> onConfirm = nullptr;
  std::function<void()> onCancel = nullptr;
  std::function<void(CSharedPointer<IWindow>)> onWindowCreated = nullptr;
};

CSharedPointer<IWindow> showConfirmDialog(const ConfirmDialogContext &ctx);

} // namespace UI::Dialogs

#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <functional>
#include <string>
#include <vector>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct ActionMenuContext {
  std::string title = ""; // Empty string for compact titleless popup menu
  std::vector<std::string> options;
  std::function<void(size_t index, const std::string &option)> onSelect = nullptr;

  Hyprutils::Math::Vector2D anchorPos = {-1.0, -1.0};
  Hyprutils::Math::Vector2D anchorSize = {0.0, 0.0};

  CSharedPointer<IWindow> parentWindow = nullptr;
  CSharedPointer<IBackend> backend = nullptr;
  CSharedPointer<CPalette> palette = nullptr;
  std::string fontFamily = "";

  std::function<void(CSharedPointer<IWindow>)> onWindowCreated = nullptr;
};

CSharedPointer<IWindow> showActionMenuDialog(const ActionMenuContext &ctx);

} // namespace UI::Dialogs

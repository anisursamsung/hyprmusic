#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <functional>
#include <string>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct ModalWindowResult {
  CSharedPointer<IWindow> window;
  CSharedPointer<CRectangleElement> card;
  CSharedPointer<CColumnLayoutElement> contentLayout;
};

class BaseDialog {
public:
  // Creates a centered popup window with a base card and content layout
  static ModalWindowResult createCenteredModal(
      CSharedPointer<IWindow> parentWindow,
      const Hyprutils::Math::Vector2D &size,
      CSharedPointer<CPalette> palette,
      float rounding = 10.0f,
      int layoutGap = 12,
      int layoutMargin = 15);

  // Creates an anchored popup window with automatic gravity calculation (below/above, left/right)
  static ModalWindowResult createAnchoredPopup(
      CSharedPointer<IWindow> parentWindow,
      Hyprutils::Math::Vector2D anchorPos,
      Hyprutils::Math::Vector2D anchorSize,
      const Hyprutils::Math::Vector2D &popupSize,
      CSharedPointer<CPalette> palette,
      float rounding = 8.0f,
      int layoutGap = 8,
      int layoutMargin = 8);

  // Standard Header element (Icon + Title)
  static CSharedPointer<CTextElement> createHeader(
      const std::string &title,
      const std::string &icon,
      CSharedPointer<CPalette> palette,
      const std::string &fontFamily,
      CFontSize::eSizingBase size = CFontSize::HT_FONT_H3);

  // Standard Action Buttons Row (e.g. Cancel + Confirm)
  static CSharedPointer<CRowLayoutElement> createButtonRow(
      const std::string &cancelLabel,
      std::function<void()> onCancel,
      const std::string &confirmLabel,
      std::function<void()> onConfirm,
      CSharedPointer<CPalette> palette,
      const std::string &fontFamily,
      bool confirmIsAccent = true,
      int gap = 20);
};

} // namespace UI::Dialogs

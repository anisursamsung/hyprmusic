#pragma once
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Line.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <functional>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class UIFactory {
public:
  static CSharedPointer<CTextElement> createHeader(
      const std::string &text, CSharedPointer<CPalette> palette,
      const std::string &fontFamily, CFontSize::eSizingBase size = CFontSize::HT_FONT_H3);

  static CSharedPointer<CTextElement> createActionButton(
      const std::string &label, std::function<void()> onClick,
      CSharedPointer<CPalette> palette, const std::string &fontFamily,
      bool isAccent = true);

  static CSharedPointer<CTextboxElement> createSearchInput(
      const std::string &placeholder, const std::string &defaultText,
      std::function<void(const std::string &)> onEdited,
      CSharedPointer<CPalette> palette, const std::string &fontFamily);

  static CSharedPointer<CRectangleElement> createCard(
      CSharedPointer<CPalette> palette, float rounding = 10.0f,
      CHyprColor fallbackColor = CHyprColor(0.12, 0.12, 0.12, 1.0));
};

} // namespace UI::Components

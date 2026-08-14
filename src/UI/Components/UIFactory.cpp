#include "UIFactory.hpp"

namespace UI::Components {

CSharedPointer<CTextElement> UIFactory::createHeader(
    const std::string &text, CSharedPointer<CPalette> palette,
    const std::string &fontFamily, CFontSize::eSizingBase size) {
  return CTextBuilder::begin()
      ->text(std::string(text))
      ->color([palette] {
        return palette ? palette->m_colors.text
                       : CHyprColor(1.0, 1.0, 1.0, 1.0);
      })
      ->fontFamily(std::string(fontFamily))
      ->fontSize(CFontSize(size))
      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                          CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
      ->commence();
}

CSharedPointer<CTextElement> UIFactory::createActionButton(
    const std::string &label, std::function<void()> onClick,
    CSharedPointer<CPalette> palette, const std::string &fontFamily,
    bool isAccent) {
  auto btn =
      CTextBuilder::begin()
          ->text(std::string(label))
          ->color([palette, isAccent] {
            if (isAccent) {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            }
            return palette ? palette->m_colors.text
                           : CHyprColor(0.7, 0.7, 0.7, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->interactable(true)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  btn->setReceivesMouse(true);
  btn->setMouseButton(
      [onClick](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (onClick)
            onClick();
        }
      });
  return btn;
}

CSharedPointer<CTextboxElement> UIFactory::createSearchInput(
    const std::string &placeholder, const std::string &defaultText,
    std::function<void(const std::string &)> onEdited,
    CSharedPointer<CPalette> palette, const std::string &fontFamily) {
  (void)palette;
  (void)fontFamily;
  return CTextboxBuilder::begin()
      ->placeholder(std::string(placeholder))
      ->defaultText(std::string(defaultText))
      ->onTextEdited(
          [onEdited](CSharedPointer<CTextboxElement>, const std::string &text) {
            if (onEdited)
              onEdited(text);
          })
      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                          CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
      ->commence();
}

CSharedPointer<CRectangleElement> UIFactory::createCard(
    CSharedPointer<CPalette> palette, float rounding,
    CHyprColor fallbackColor) {
  return CRectangleBuilder::begin()
      ->color([palette, fallbackColor] {
        return palette ? palette->m_colors.base : fallbackColor;
      })
      ->rounding(rounding)
      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                          CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
      ->commence();
}

} // namespace UI::Components

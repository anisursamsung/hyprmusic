#include "CenteredTextLabel.hpp"

namespace UI::Components {

CenteredTextLabel::CenteredTextLabel(const CenteredTextLabelContext &ctx)
    : m_ctx(ctx) {}

CSharedPointer<CColumnLayoutElement> CenteredTextLabel::build() {
  m_container =
      CColumnLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  m_container->setMargin(m_ctx.margin);

  auto palette = m_ctx.palette;
  colorFn textColor = m_ctx.color;
  if (!textColor) {
    textColor = [palette] {
      return palette ? palette->m_colors.accent
                     : CHyprColor(0.2, 0.8, 0.4, 1.0);
    };
  }

  m_textElement =
      CTextBuilder::begin()
          ->text(std::string(m_ctx.text))
          ->color(std::move(textColor))
          ->fontFamily(std::string(m_ctx.fontFamily))
          ->fontSize(CFontSize(m_ctx.fontSize))
          ->align(HT_FONT_ALIGN_CENTER)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_textElement->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  m_textElement->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
  m_textElement->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

  m_container->addChild(m_textElement);
  return m_container;
}

void CenteredTextLabel::updateText(const std::string &newText) {
  m_ctx.text = newText;
  if (m_textElement) {
    m_textElement->rebuild()
        ->text(std::string(newText))
        ->align(HT_FONT_ALIGN_CENTER)
        ->noEllipsize(false)
        ->fontSize(CFontSize(m_ctx.fontSize))
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                            CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
        ->commence();
  }
}

} // namespace UI::Components

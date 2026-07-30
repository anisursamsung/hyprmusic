#pragma once
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <functional>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct CenteredTextLabelContext {
  std::string text;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  CFontSize fontSize = CFontSize(CFontSize::HT_FONT_TEXT);
  colorFn color = nullptr;
  float margin = 0.0f;
};

class CenteredTextLabel {
public:
  explicit CenteredTextLabel(const CenteredTextLabelContext &ctx);

  CSharedPointer<CColumnLayoutElement> build();
  void updateText(const std::string &newText);

  CSharedPointer<CTextElement> getTextElement() const { return m_textElement; }
  CSharedPointer<CColumnLayoutElement> getContainer() const { return m_container; }

private:
  CenteredTextLabelContext m_ctx;
  CSharedPointer<CColumnLayoutElement> m_container;
  CSharedPointer<CTextElement> m_textElement;
};

} // namespace UI::Components

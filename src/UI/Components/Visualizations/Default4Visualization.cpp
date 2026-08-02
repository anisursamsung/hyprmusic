#include "Default4Visualization.hpp"
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

const int HORIZONTAL_BAR_COUNT = 16;

CSharedPointer<IElement> Default4Visualization::build(CSharedPointer<CPalette> palette) {
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  // Use a ColumnLayout instead of RowLayout to stack them vertically
  auto barCol = CColumnLayoutBuilder::begin()
                    ->gap(8)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();
  barCol->setMargin(20);

  m_bars.clear();
  m_lastWidths.clear();

  for (int i = 0; i < HORIZONTAL_BAR_COUNT; ++i) {
    auto rowContainer = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 1.0f / HORIZONTAL_BAR_COUNT}))
                   ->commence();

    // Create a smooth transition from Accent to Alternate Base
    CHyprColor color1 = palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
    CHyprColor color2 = palette ? palette->m_colors.text : CHyprColor(1.0, 1.0, 1.0, 1.0);
    float mixRatio = static_cast<float>(i) / (HORIZONTAL_BAR_COUNT - 1);
    CHyprColor mixedColor = color1.mix(color2, mixRatio);

    auto bar = CRectangleBuilder::begin()
                   ->color([mixedColor] { return mixedColor; })
                   ->rounding(4)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {0.05F, 0.8F})) // Width is dynamic, Height is mostly fixed
                   ->commence();

    // Anchor to the exact center of the screen
    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

    m_bars.push_back(bar);
    m_lastWidths.push_back(0.05f);

    rowContainer->addChild(bar);
    barCol->addChild(rowContainer);
  }

  m_container->addChild(barCol);
  return m_container;
}

void Default4Visualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return;

  int binsPerBar = spectrum.size() / HORIZONTAL_BAR_COUNT;

  for (size_t i = 0; i < m_bars.size(); ++i) {
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar; ++j) {
      sum += spectrum[i * binsPerBar + j];
    }
    float w = std::clamp(sum / binsPerBar, 0.05f, 1.0f);

    // Delta Gate (Checking width instead of height)
    if (std::abs(w - m_lastWidths[i]) > 0.02f) {
      m_lastWidths[i] = w;
      
      m_bars[i]->rebuild()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {w, 0.8F}))
          ->commence();
    }
  }
}

} // namespace UI::Components

#include "Default3Visualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

const int RAINBOW_BAR_COUNT = 16;

CSharedPointer<IElement> Default3Visualization::build(CSharedPointer<CPalette> palette) {
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  auto barRow = CRowLayoutBuilder::begin()
                    ->gap(8)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();
  barRow->setMargin(20);

  m_bars.clear();
  m_lastHeights.clear();

  for (int i = 0; i < RAINBOW_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / RAINBOW_BAR_COUNT, 1.0F}))
                   ->commence();

    // Generate a static rainbow gradient across the bars using Sine waves
    float freq = 0.3f;
    float r = std::sin(freq * i + 0.0f) * 0.5f + 0.5f;
    float g = std::sin(freq * i + 2.0f) * 0.5f + 0.5f;
    float b = std::sin(freq * i + 4.0f) * 0.5f + 0.5f;
    CHyprColor barColor(r, g, b, 1.0f);

    auto bar = CRectangleBuilder::begin()
                   ->color([barColor] { return barColor; })
                   ->rounding(4)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.05F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);

    m_bars.push_back(bar);
    m_lastHeights.push_back(0.05f);

    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

void Default3Visualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return;

  int binsPerBar = spectrum.size() / RAINBOW_BAR_COUNT;

  for (size_t i = 0; i < m_bars.size(); ++i) {
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar; ++j) {
      sum += spectrum[i * binsPerBar + j];
    }
    float h = std::clamp(sum / binsPerBar, 0.05f, 1.0f);

    // Delta Gate: Extremely lightweight!
    if (std::abs(h - m_lastHeights[i]) > 0.02f) {
      m_lastHeights[i] = h;
      m_bars[i]->rebuild()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, h}))
          ->commence();
    }
  }
}

} // namespace UI::Components

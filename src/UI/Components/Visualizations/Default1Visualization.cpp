#include "Default1Visualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

// Reduce from 64 to 16 for a clean, chunky look that is 4x faster to render
const int SIMPLE_BAR_COUNT = 16; 

CSharedPointer<IElement> Default1Visualization::build(CSharedPointer<CPalette> palette) {
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  auto barRow = CRowLayoutBuilder::begin()
                    ->gap(8) // Wider gap for fewer bars
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();
  barRow->setMargin(20);

  m_bars.clear();
  m_lastHeights.clear();

  for (int i = 0; i < SIMPLE_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / SIMPLE_BAR_COUNT, 1.0F}))
                   ->commence();

    auto bar = CRectangleBuilder::begin()
                   ->color([palette] {
                     return palette ? palette->m_colors.accent
                                    : CHyprColor(0.2, 0.8, 0.4, 1.0);
                   })
                   ->rounding(4) // Slightly rounder for the thicker bars
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.05F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);

    m_bars.push_back(bar);
    m_lastHeights.push_back(0.05f); // Base minimum height

    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

void Default1Visualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return;

  // Since spectrum has 64 items, each bar represents 4 bins
  int binsPerBar = spectrum.size() / SIMPLE_BAR_COUNT;

  for (size_t i = 0; i < m_bars.size(); ++i) {
    // 1. Average the spectrum data for this thicker bar
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar; ++j) {
      sum += spectrum[i * binsPerBar + j];
    }
    float avg = sum / binsPerBar;
    
    // Clamp to a minimum of 5% height
    float h = std::clamp(avg, 0.05f, 1.0f);

    // 2. DELTA GATE: Only rebuild the UI if the height changed by more than 2%
    // This stops the UI toolkit from queuing thousands of useless frame updates!
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

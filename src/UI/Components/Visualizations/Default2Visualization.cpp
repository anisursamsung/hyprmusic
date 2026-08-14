#include "Default2Visualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

// 16 bars total, perfectly symmetric
const int SYMMETRIC_BAR_COUNT = 16;
// 8 distinct frequency bins mapped to both sides of the mirror
const int MIRRORED_BINS = SYMMETRIC_BAR_COUNT / 2; 

CSharedPointer<IElement> Default2Visualization::build(CSharedPointer<CPalette> palette) {
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

  for (int i = 0; i < SYMMETRIC_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / SYMMETRIC_BAR_COUNT, 1.0F}))
                   ->commence();

    auto bar = CRectangleBuilder::begin()
                   ->color([palette] {
                     return palette ? palette->m_colors.text
                                    : CHyprColor(1.0, 1.0, 1.0, 1.0);
                   })
                   ->rounding(4)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.05F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    
    // ---> The Magic: Center alignment makes it pulse UP and DOWN symmetrically <---
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true); 

    m_bars.push_back(bar);
    m_lastHeights.push_back(0.05f); // Base minimum height

    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

void Default2Visualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return;

  // 1. Downsample the 64 raw bins into 8 mirrored bins
  int rawBinsPerMirroredBin = spectrum.size() / MIRRORED_BINS;

  std::vector<float> averagedBins(MIRRORED_BINS, 0.0f);
  for (int i = 0; i < MIRRORED_BINS; ++i) {
    float sum = 0.0f;
    for (int j = 0; j < rawBinsPerMirroredBin; ++j) {
      sum += spectrum[i * rawBinsPerMirroredBin + j];
    }
    averagedBins[i] = sum / rawBinsPerMirroredBin;
  }

  // 2. Apply to the 16 bars
  for (size_t i = 0; i < m_bars.size(); ++i) {
    
    // Map the 16 bars to the 8 bins so that Bass (0) is in the middle, and Treble (7) is on the edges
    int binIndex = (i < MIRRORED_BINS) ? (MIRRORED_BINS - 1 - i) : (i - MIRRORED_BINS);
    
    float avg = averagedBins[binIndex];
    float h = std::clamp(avg, 0.05f, 1.0f);

    // 3. DELTA GATE: Only rebuild the UI if the height changed by more than 2%
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

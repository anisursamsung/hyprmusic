#include "PeakHoldNeonVisualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

const int NEON_BAR_COUNT = 16;

CSharedPointer<IElement> PeakHoldNeonVisualization::build(CSharedPointer<CPalette> palette) {
  (void)palette;
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  auto barRow = CRowLayoutBuilder::begin()
                    ->gap(4)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();
  barRow->setMargin(20);

  m_bars.clear();
  m_peaks.clear();
  m_lastHeights.clear();
  m_peakHeights.clear();
  m_peakVelocities.clear();

  for (int i = 0; i < NEON_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / NEON_BAR_COUNT, 1.0F}))
                   ->commence();

    // Vibrant Electric Neon Gradient: Teal (#00F2FE) -> Violet (#9B51E0) -> Pink (#FF0844)
    float t = static_cast<float>(i) / static_cast<float>(NEON_BAR_COUNT - 1);
    float r = 0.1f + 0.8f * std::sin(t * M_PI);
    float g = 0.8f * (1.0f - t);
    float b = 0.5f + 0.5f * t;
    CHyprColor barColor(r, g, b, 0.85f);

    // Main Audio Bar
    auto bar = CRectangleBuilder::begin()
                   ->color([barColor] { return barColor; })
                   ->rounding(3)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.05F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);

    // Floating Peak Indicator Cap
    float brightR = std::min(1.0f, static_cast<float>(barColor.r) + 0.3f);
    float brightG = std::min(1.0f, static_cast<float>(barColor.g) + 0.3f);
    CHyprColor peakColor(brightR, brightG, 1.0f, 1.0f);

    auto peakCap = CRectangleBuilder::begin()
                       ->color([peakColor] { return peakColor; })
                       ->rounding(2)
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_ABSOLUTE,
                                           {1.0F, 3.0F}))
                       ->commence();

    peakCap->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    peakCap->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    peakCap->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);

    m_bars.push_back(bar);
    m_peaks.push_back(peakCap);
    m_lastHeights.push_back(0.05f);
    m_peakHeights.push_back(0.05f);
    m_peakVelocities.push_back(0.0f);

    col->addChild(bar);
    col->addChild(peakCap);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

bool PeakHoldNeonVisualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return false;

  int binsPerBar = std::max(1, static_cast<int>(spectrum.size()) / NEON_BAR_COUNT);
  bool updated = false;

  for (size_t i = 0; i < m_bars.size(); ++i) {
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar && (i * binsPerBar + j) < spectrum.size(); ++j) {
      sum += spectrum[i * binsPerBar + j];
    }
    float h = std::clamp(sum / binsPerBar, 0.05f, 1.0f);

    // Peak-Hold Physics Calculation
    if (h >= m_peakHeights[i]) {
      m_peakHeights[i] = h;
      m_peakVelocities[i] = 0.0f;
    } else {
      m_peakVelocities[i] += 0.004f; // Gravity acceleration
      m_peakHeights[i] -= m_peakVelocities[i];
      if (m_peakHeights[i] < h) {
        m_peakHeights[i] = h;
        m_peakVelocities[i] = 0.0f;
      }
    }

    // Rebuild main bar if height changed meaningfully
    if (std::abs(h - m_lastHeights[i]) > 0.03f) {
      m_lastHeights[i] = h;
      m_bars[i]->rebuild()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, h}))
          ->commence();
      updated = true;
    }
  }
  return updated;
}

} // namespace UI::Components

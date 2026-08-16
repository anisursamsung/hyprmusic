#include "CircularRadialVisualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

const int RADIAL_BAR_COUNT = 20;

CSharedPointer<IElement> CircularRadialVisualization::build(CSharedPointer<CPalette> palette) {
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
  barRow->setMargin(25);

  m_bars.clear();
  m_lastHeights.clear();
  m_velocities.clear();

  for (int i = 0; i < RADIAL_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / RADIAL_BAR_COUNT, 1.0F}))
                   ->commence();

    // Cosmic Palette: Amber Gold -> Electric Magenta -> Cosmic Cyan
    float norm = static_cast<float>(i) / static_cast<float>(RADIAL_BAR_COUNT - 1);
    float r = 1.0f - 0.7f * norm;
    float g = 0.3f + 0.6f * std::sin(norm * M_PI);
    float b = 0.2f + 0.8f * norm;
    CHyprColor barColor(r, g, b, 0.9F);

    auto bar = CRectangleBuilder::begin()
                   ->color([barColor] { return barColor; })
                   ->rounding(4)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.05F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

    m_bars.push_back(bar);
    m_lastHeights.push_back(0.05f);
    m_velocities.push_back(0.0f);

    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

bool CircularRadialVisualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return false;

  int binsPerBar = std::max(1, static_cast<int>(spectrum.size()) / RADIAL_BAR_COUNT);
  bool updated = false;

  for (int i = 0; i < RADIAL_BAR_COUNT; ++i) {
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar; ++j) {
      int idx = i * binsPerBar + j;
      if (idx < static_cast<int>(spectrum.size())) {
        sum += spectrum[idx];
      }
    }
    float targetH = std::clamp((sum / binsPerBar) * 0.90f, 0.05f, 0.48f);

    // Dynamic Pulsing Physics
    float force = (targetH - m_lastHeights[i]) * 0.45f;
    m_velocities[i] = (m_velocities[i] + force) * 0.72f;
    float newH = std::clamp(m_lastHeights[i] + m_velocities[i], 0.05f, 0.48f);

    if (std::abs(newH - m_lastHeights[i]) > 0.025f) {
      m_lastHeights[i] = newH;
      m_bars[i]->rebuild()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, m_lastHeights[i] * 2.0f}))
          ->commence();
      updated = true;
    }
  }
  return updated;
}

} // namespace UI::Components

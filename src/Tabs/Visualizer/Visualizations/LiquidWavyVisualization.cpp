#include "LiquidWavyVisualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

const int WAVY_BAR_COUNT = 24;

CSharedPointer<IElement> LiquidWavyVisualization::build(CSharedPointer<CPalette> palette) {
  (void)palette;
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  auto barRow = CRowLayoutBuilder::begin()
                    ->gap(2)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();
  barRow->setMargin(15);

  m_bars.clear();
  m_heights.clear();
  m_velocities.clear();

  for (int i = 0; i < WAVY_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / WAVY_BAR_COUNT, 1.0F}))
                   ->commence();

    // Fluid Wavy Color Spectrum: Electric Aqua -> Emerald -> Magenta -> Sunset Coral
    float phase = static_cast<float>(i) / static_cast<float>(WAVY_BAR_COUNT - 1) * M_PI * 2.0f;
    float r = 0.2f + 0.7f * std::sin(phase + 0.0f) * 0.5f + 0.35f;
    float g = 0.2f + 0.7f * std::sin(phase + 2.1f) * 0.5f + 0.35f;
    float b = 0.3f + 0.6f * std::sin(phase + 4.2f) * 0.5f + 0.3f;
    CHyprColor barColor(r, g, b, 0.9f);

    auto bar = CRectangleBuilder::begin()
                   ->color([barColor] { return barColor; })
                   ->rounding(2)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.03F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

    m_bars.push_back(bar);
    m_heights.push_back(0.03f);
    m_velocities.push_back(0.0f);

    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

bool LiquidWavyVisualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return false;

  // 1. Spatial Wave Smoothing across bars
  std::vector<float> raw(WAVY_BAR_COUNT, 0.03f);
  int binsPerBar = std::max(1, static_cast<int>(spectrum.size()) / WAVY_BAR_COUNT);

  for (int i = 0; i < WAVY_BAR_COUNT; ++i) {
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar; ++j) {
      int idx = i * binsPerBar + j;
      if (idx < static_cast<int>(spectrum.size())) {
        sum += spectrum[idx];
      }
    }
    raw[i] = std::clamp(sum / binsPerBar, 0.03f, 0.48f);
  }

  std::vector<float> targetHeights(WAVY_BAR_COUNT, 0.03f);
  for (int i = 0; i < WAVY_BAR_COUNT; ++i) {
    float prev = (i > 0) ? raw[i - 1] : raw[i];
    float next = (i < WAVY_BAR_COUNT - 1) ? raw[i + 1] : raw[i];
    targetHeights[i] = 0.22f * prev + 0.56f * raw[i] + 0.22f * next;
  }

  bool updated = false;
  // 2. Liquid Spring & Gravity Physics Update
  for (int i = 0; i < WAVY_BAR_COUNT; ++i) {
    float force = (targetHeights[i] - m_heights[i]) * 0.38f;
    m_velocities[i] = (m_velocities[i] + force) * 0.70f;
    float newH = std::clamp(m_heights[i] + m_velocities[i], 0.03f, 0.48f);

    if (std::abs(newH - m_heights[i]) > 0.025f) {
      m_heights[i] = newH;
      m_bars[i]->rebuild()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, m_heights[i] * 2.0f}))
          ->commence();
      updated = true;
    }
  }
  return updated;
}

} // namespace UI::Components

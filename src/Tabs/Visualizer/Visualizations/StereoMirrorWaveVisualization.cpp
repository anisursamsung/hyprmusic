#include "StereoMirrorWaveVisualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

const int MIRROR_BAR_COUNT = 24;

CSharedPointer<IElement> StereoMirrorWaveVisualization::build(CSharedPointer<CPalette> palette) {
  (void)palette;
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  auto barRow = CRowLayoutBuilder::begin()
                    ->gap(3)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();
  barRow->setMargin(15);

  m_bars.clear();
  m_lastHeights.clear();
  m_velocities.clear();

  int halfCount = MIRROR_BAR_COUNT / 2;

  for (int i = 0; i < MIRROR_BAR_COUNT; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); })
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / MIRROR_BAR_COUNT, 1.0F}))
                   ->commence();

    // Mirror distance from center
    int distFromCenter = std::abs(i - halfCount);
    float t = static_cast<float>(distFromCenter) / static_cast<float>(halfCount);

    // Neon Gradient: Center Electric Cyan (#00E5FF) -> Violet -> Hot Coral Edge
    float r = 0.0f + 0.95f * t;
    float g = 0.85f * (1.0f - t * 0.7f);
    float b = 1.0f - 0.4f * t;
    CHyprColor barColor(r, g, b, 0.9F);

    auto bar = CRectangleBuilder::begin()
                   ->color([barColor] { return barColor; })
                   ->rounding(3)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.04F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

    m_bars.push_back(bar);
    m_lastHeights.push_back(0.04f);
    m_velocities.push_back(0.0f);

    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

bool StereoMirrorWaveVisualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty() || spectrum.empty()) return false;

  int halfCount = std::max(1, MIRROR_BAR_COUNT / 2);
  int binsPerBar = std::max(1, static_cast<int>(spectrum.size()) / halfCount);
  bool updated = false;

  // Map frequency spectrum to center-outward stereo mirror layout
  for (int i = 0; i < MIRROR_BAR_COUNT; ++i) {
    int dist = std::abs(i - halfCount);
    float sum = 0.0f;
    for (int j = 0; j < binsPerBar; ++j) {
      int idx = dist * binsPerBar + j;
      if (idx < static_cast<int>(spectrum.size())) {
        sum += spectrum[idx];
      }
    }
    float targetH = std::clamp((sum / binsPerBar) * 0.85f, 0.04f, 0.46f);

    // Inertial Spring Momentum
    float force = (targetH - m_lastHeights[i]) * 0.42f;
    m_velocities[i] = (m_velocities[i] + force) * 0.74f;
    float newH = std::clamp(m_lastHeights[i] + m_velocities[i], 0.04f, 0.46f);

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

#include "Default1Visualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <algorithm>

namespace UI::Components {

CSharedPointer<IElement> Default1Visualization::build(CSharedPointer<CPalette> palette) {
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
  for (int i = 0; i < 64; ++i) {
    auto col = CRectangleBuilder::begin()
                   ->color([] { return CHyprColor(0, 0, 0, 0); }) 
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0f / 64.0f, 1.0F}))
                   ->commence();

    auto bar = CRectangleBuilder::begin()
                   ->color([palette] {
                     return palette ? palette->m_colors.accent 
                                    : CHyprColor(0.2, 0.8, 0.4, 1.0);
                   })
                   ->rounding(2)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT,
                                       {1.0F, 0.05F}))
                   ->commence();

    bar->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    bar->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);

    m_bars.push_back(bar);
    col->addChild(bar);
    barRow->addChild(col);
  }

  m_container->addChild(barRow);
  return m_container;
}

void Default1Visualization::update(const std::vector<float>& spectrum) {
  if (m_bars.empty()) return;
  for (size_t i = 0; i < m_bars.size(); ++i) {
    float h = std::max(0.02f, spectrum[i]); 
    m_bars[i]->rebuild()
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_PERCENT,
                            {1.0F, h}))
        ->commence();
  }
}

} // namespace UI::Components

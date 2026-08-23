#pragma once
#include "IVisualization.hpp"
#include <hyprtoolkit/element/Rectangle.hpp>
#include <vector>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class RainbowSpectrumVisualization : public IVisualization {
public:
  CSharedPointer<IElement> build(CSharedPointer<CPalette> palette) override;
  bool update(const std::vector<float>& spectrum) override;

private:
  CSharedPointer<CRectangleElement> m_container;
  std::vector<CSharedPointer<CRectangleElement>> m_bars;
  std::vector<float> m_lastHeights;
};

} // namespace UI::Components

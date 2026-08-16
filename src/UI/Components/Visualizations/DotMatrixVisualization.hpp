#pragma once
#include "IVisualization.hpp"
#include <hyprtoolkit/element/Rectangle.hpp>
#include <vector>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class DotMatrixVisualization : public IVisualization {
public:
  CSharedPointer<IElement> build(CSharedPointer<CPalette> palette) override;
  bool update(const std::vector<float>& spectrum) override;

private:
  CSharedPointer<CRectangleElement> m_container;
  // Grid of dots stored column by column (16 cols x 10 rows)
  std::vector<std::vector<CSharedPointer<CRectangleElement>>> m_dotGrid;
  std::vector<float> m_lastColHeights;
};

} // namespace UI::Components

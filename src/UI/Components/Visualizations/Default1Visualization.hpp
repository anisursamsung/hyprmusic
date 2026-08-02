#pragma once
#include "IVisualization.hpp"
#include <hyprtoolkit/element/Rectangle.hpp>
#include <vector>

namespace UI::Components {

class Default1Visualization : public IVisualization {
public:
  CSharedPointer<IElement> build(CSharedPointer<CPalette> palette) override;
  void update(const std::vector<float>& spectrum) override;

private:
  CSharedPointer<CRectangleElement> m_container;
  std::vector<CSharedPointer<CRectangleElement>> m_bars;
  
  // ---> NEW: Track last heights to prevent UI spam <---
  std::vector<float> m_lastHeights;
};

} // namespace UI::Components

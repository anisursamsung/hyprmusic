#pragma once
#include "IVisualization.hpp"
#include <hyprtoolkit/element/Rectangle.hpp>
#include <vector>

namespace UI::Components {

class Default2Visualization : public IVisualization {
public:
  CSharedPointer<IElement> build(CSharedPointer<CPalette> palette) override;
  void update(const std::vector<float>& spectrum) override;

private:
  CSharedPointer<CRectangleElement> m_container;
  std::vector<CSharedPointer<CRectangleElement>> m_bars;
  
  // Track last heights to prevent UI thread spam (The Delta Gate)
  std::vector<float> m_lastHeights;
};

} // namespace UI::Components

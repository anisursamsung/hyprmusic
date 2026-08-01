#pragma once
#include "IVisualization.hpp"
#include <hyprtoolkit/element/Rectangle.hpp>

namespace UI::Components {

struct KineticBall {
  CSharedPointer<CRectangleElement> elem;
  float x, y;
  float dx, dy;
  float baseSize;
  CHyprColor color; // <-- New property for individual colors
};

class Default2Visualization : public IVisualization {
public:
  CSharedPointer<IElement> build(CSharedPointer<CPalette> palette) override;
  void update(const std::vector<float>& spectrum) override;

private:
  CSharedPointer<CRectangleElement> m_container;
  std::vector<KineticBall> m_balls;
};

} // namespace UI::Components

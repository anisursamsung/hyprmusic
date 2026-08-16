#pragma once

#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <vector>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class IVisualization {
public:
  virtual ~IVisualization() = default;

  // Builds the UI tree for this specific visualization
  virtual CSharedPointer<IElement> build(CSharedPointer<CPalette> palette) = 0;

  // Called periodically with the smoothed frequency spectrum; returns true if any bar was updated
  virtual bool update(const std::vector<float>& spectrum) = 0;
};

} // namespace UI::Components

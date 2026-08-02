#pragma once

#include "../Components/Visualizer.hpp"
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <memory>

namespace UI::Views {
using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct VisualizerViewContext {
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
};

class VisualizerView {
public:
  explicit VisualizerView(const VisualizerViewContext &ctx);
  void rebuildUI(CSharedPointer<CRectangleElement> wrapper);
  void destroyVisualizer();

private:
  VisualizerViewContext m_ctx;
  CSharedPointer<CRectangleElement> m_tabContentWrapper;
  std::shared_ptr<UI::Components::Visualizer> m_visualizer;
};

} // namespace UI::Views

#include "VisualizerView.hpp"

namespace UI::Views {

VisualizerView::VisualizerView(const VisualizerViewContext &ctx) : m_ctx(ctx) {}

void VisualizerView::rebuildUI(CSharedPointer<CRectangleElement> wrapper) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  m_tabContentWrapper->clearChildren();

  // Instantiate the visualizer if it doesn't exist
  if (!m_visualizer) {
    m_visualizer = std::make_shared<UI::Components::Visualizer>(m_ctx.backend, m_ctx.palette);
  }

  m_tabContentWrapper->addChild(m_visualizer->build());
  m_tabContentWrapper->forceReposition();
}

void VisualizerView::destroyVisualizer() {
  // Free memory and stop the background thread when leaving the tab
  if (m_visualizer) {
    m_visualizer.reset();
  }
}

} // namespace UI::Views

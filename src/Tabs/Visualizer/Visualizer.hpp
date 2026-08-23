#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Visualizations/IVisualization.hpp"

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

// Encapsulate shared state so the detached thread can clean itself up safely
struct VisualizerSharedData {
  std::atomic<bool> running{true};
  std::mutex dataMutex;
  std::vector<float> smoothedSpectrum;
  int fifoFd{-1};
};

class Visualizer : public std::enable_shared_from_this<Visualizer> {
public:
  Visualizer(CSharedPointer<IBackend> backend, CSharedPointer<CPalette> palette);
  ~Visualizer();

  CSharedPointer<IElement> build();

private:
  // Static thread function takes ownership of the shared state
  static void readerThreadFunc(std::shared_ptr<VisualizerSharedData> sharedData);
  void scheduleUpdate();
  void cycleVisualization();

  CSharedPointer<IBackend> m_backend;
  CSharedPointer<CPalette> m_palette;
  CSharedPointer<CRectangleElement> m_container;
  
  std::vector<std::shared_ptr<IVisualization>> m_visualizations;
  size_t m_currentIndex = 0;

  std::vector<float> m_spectrumBuffer;
  std::shared_ptr<VisualizerSharedData> m_sharedData;
};

} // namespace UI::Components

#include "Visualizer.hpp"
#include "Visualizations/Default1Visualization.hpp"
#include "Visualizations/Default2Visualization.hpp"
#include "Visualizations/Default3Visualization.hpp" 
#include "Visualizations/Default4Visualization.hpp"
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <algorithm>
#include <iostream>

namespace UI::Components {

const int BARS_COUNT = 64;
const int SAMPLES_PER_FRAME = 512;

Visualizer::Visualizer(CSharedPointer<IBackend> backend, CSharedPointer<CPalette> palette)
    : m_backend(backend), m_palette(palette) {
    m_visualizations.push_back(std::make_shared<Default3Visualization>());
  m_visualizations.push_back(std::make_shared<Default4Visualization>());
 m_visualizations.push_back(std::make_shared<Default1Visualization>());
  m_visualizations.push_back(std::make_shared<Default2Visualization>());


  m_sharedData = std::make_shared<VisualizerSharedData>();
  m_sharedData->smoothedSpectrum.resize(BARS_COUNT, 0.0f);
  
  std::thread(&Visualizer::readerThreadFunc, m_sharedData).detach();
}

Visualizer::~Visualizer() {
  if (m_sharedData) {
    m_sharedData->running = false;
  }
}

void Visualizer::readerThreadFunc(std::shared_ptr<VisualizerSharedData> sharedData) {
  // ---> MATH BOTTLENECK FIX: PRE-COMPUTE LOOKUP TABLES <---
  // Calculate the Hanning window and all Sine/Cosine angles exactly ONCE.
  std::vector<float> window(SAMPLES_PER_FRAME);
  for (int n = 0; n < SAMPLES_PER_FRAME; ++n) {
    window[n] = 0.5f * (1.0f - std::cos(2.0f * M_PI * n / (SAMPLES_PER_FRAME - 1)));
  }

  std::vector<std::vector<float>> cosTable(BARS_COUNT, std::vector<float>(SAMPLES_PER_FRAME));
  std::vector<std::vector<float>> sinTable(BARS_COUNT, std::vector<float>(SAMPLES_PER_FRAME));

  for (int k = 0; k < BARS_COUNT; ++k) {
    float freqIndex = std::pow(static_cast<float>(k) / BARS_COUNT, 2.0f) * (SAMPLES_PER_FRAME / 2.0f);
    for (int n = 0; n < SAMPLES_PER_FRAME; ++n) {
      float angle = 2.0f * M_PI * freqIndex * n / SAMPLES_PER_FRAME;
      cosTable[k][n] = std::cos(angle);
      sinTable[k][n] = std::sin(angle);
    }
  }
  // --------------------------------------------------------

  sharedData->fifoFd = open("/tmp/mpd.fifo", O_RDWR | O_NONBLOCK);
  if (sharedData->fifoFd < 0) {
    std::cerr << "Visualizer: Failed to open /tmp/mpd.fifo" << std::endl;
    return;
  }

  // Initial flush: dump any stale data sitting in the pipe from before the app launched
  char discard[4096];
  while (read(sharedData->fifoFd, discard, sizeof(discard)) > 0) {}

  const int bytesPerSample = 2 * 2; 
  int bytesToRead = SAMPLES_PER_FRAME * bytesPerSample;

  while (sharedData->running) {
    struct pollfd pfd;
    pfd.fd = sharedData->fifoFd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 50);

    if (ret > 0 && (pfd.revents & POLLIN)) {
      std::vector<int16_t> tempBuffer(SAMPLES_PER_FRAME * 2);
      std::vector<int16_t> finalBuffer(SAMPLES_PER_FRAME * 2);
      int lastBytesRead = 0;

      // THE FIX: Rapidly drain the pipe to clear the backlog, keeping ONLY the very last chunk.
      // This prevents the thread from locking the UI while processing stale data.
      while (true) {
        int br = read(sharedData->fifoFd, tempBuffer.data(), bytesToRead);
        if (br > 0) {
          finalBuffer = tempBuffer;
          lastBytesRead = br;
        } else {
          // br == -1 (EAGAIN) meaning the pipe is now completely empty and we caught up to live time
          break; 
        }
      }

      // Only perform the math on the absolute newest frame
      if (lastBytesRead > 0) {
        int samplesRead = lastBytesRead / bytesPerSample;
        // Safety bound to prevent array overflow on partial reads
        if (samplesRead > SAMPLES_PER_FRAME) samplesRead = SAMPLES_PER_FRAME;

        std::vector<float> currentSpectrum(BARS_COUNT, 0.0f);
        std::vector<float> monoSamples(samplesRead);
        
        for (int i = 0; i < samplesRead; ++i) {
          float left = finalBuffer[i * 2] / 32768.0f;
          float right = finalBuffer[i * 2 + 1] / 32768.0f;
          monoSamples[i] = (left + right) / 2.0f;
        }

        // ---> FAST ARRAY LOOKUPS <---
        for (int k = 0; k < BARS_COUNT; ++k) {
          float re = 0.0f;
          float im = 0.0f;
          
          for (int n = 0; n < samplesRead; ++n) {
            float val = monoSamples[n] * window[n];
            re += val * cosTable[k][n];
            im -= val * sinTable[k][n];
          }
          
          float magnitude = std::sqrt(re * re + im * im) / (samplesRead / 4.0f);
          currentSpectrum[k] = std::clamp(magnitude * 2.5f, 0.0f, 1.0f);
        }

        // Lock exactly ONCE per update cycle, entirely eliminating UI thread starvation
        std::lock_guard<std::mutex> lock(sharedData->dataMutex);
        for (int i = 0; i < BARS_COUNT; ++i) {
          if (currentSpectrum[i] > sharedData->smoothedSpectrum[i]) {
            sharedData->smoothedSpectrum[i] = sharedData->smoothedSpectrum[i] * 0.2f + currentSpectrum[i] * 0.8f;
          } else {
            sharedData->smoothedSpectrum[i] = sharedData->smoothedSpectrum[i] * 0.85f + currentSpectrum[i] * 0.15f;
          }
        }
      }
    } else {
      std::lock_guard<std::mutex> lock(sharedData->dataMutex);
      for (int i = 0; i < BARS_COUNT; ++i) {
        sharedData->smoothedSpectrum[i] *= 0.90f;
      }
    }
  }

  if (sharedData->fifoFd >= 0) {
    close(sharedData->fifoFd);
  }
}

CSharedPointer<IElement> Visualizer::build() {
  m_container =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0.85f); }) 
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, 1.0F}))
          ->commence();

  m_container->setReceivesMouse(true);
  m_container->setMouseButton([this](Input::eMouseButton button, bool down) {
    if (button == Input::MOUSE_BUTTON_LEFT && !down) {
      cycleVisualization();
    }
  });

  if (!m_visualizations.empty()) {
    m_container->addChild(m_visualizations[m_currentIndex]->build(m_palette));
  }
  
  scheduleUpdate();
  return m_container;
}

void Visualizer::cycleVisualization() {
  if (m_visualizations.empty()) return;

  std::lock_guard<std::mutex> lock(m_sharedData->dataMutex);
  m_currentIndex = (m_currentIndex + 1) % m_visualizations.size();
  
  m_container->clearChildren();
  m_container->addChild(m_visualizations[m_currentIndex]->build(m_palette));
  m_container->forceReposition();
}

void Visualizer::scheduleUpdate() {
  if (!m_backend) return;

  std::weak_ptr<Visualizer> weakThis = weak_from_this();

  m_backend->addTimer(
      std::chrono::milliseconds(16), 
      [weakThis](CAtomicSharedPointer<CTimer>, void *) {
        auto sharedThis = weakThis.lock();
        if (!sharedThis || !sharedThis->m_sharedData->running) return;

        std::vector<float> snapshot;
        {
          std::lock_guard<std::mutex> lock(sharedThis->m_sharedData->dataMutex);
          snapshot = sharedThis->m_sharedData->smoothedSpectrum;
        }

        if (!sharedThis->m_visualizations.empty()) {
            sharedThis->m_visualizations[sharedThis->m_currentIndex]->update(snapshot);
        }
        
        sharedThis->m_container->forceReposition();
        sharedThis->scheduleUpdate();
      },
      nullptr);
}

} // namespace UI::Components

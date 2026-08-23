#include "Visualizer.hpp"
#include "Visualizations/RainbowSpectrumVisualization.hpp"
#include "Visualizations/PeakHoldNeonVisualization.hpp"
#include "Visualizations/LiquidWavyVisualization.hpp"
#include "Visualizations/StereoMirrorWaveVisualization.hpp"
#include "Visualizations/CircularRadialVisualization.hpp"
#include "Visualizations/DotMatrixVisualization.hpp"
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <algorithm>

namespace UI::Components {

const int BARS_COUNT = 32;
const int SAMPLES_PER_FRAME = 512;

Visualizer::Visualizer(CSharedPointer<IBackend> backend, CSharedPointer<CPalette> palette)
    : m_backend(backend), m_palette(palette) {
  m_visualizations.push_back(std::make_shared<RainbowSpectrumVisualization>());
  m_visualizations.push_back(std::make_shared<PeakHoldNeonVisualization>());
  m_visualizations.push_back(std::make_shared<LiquidWavyVisualization>());
  m_visualizations.push_back(std::make_shared<StereoMirrorWaveVisualization>());
  m_visualizations.push_back(std::make_shared<CircularRadialVisualization>());
  m_visualizations.push_back(std::make_shared<DotMatrixVisualization>());

  m_spectrumBuffer.resize(BARS_COUNT, 0.0f);
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
  // ---> HIGH PERFORMANCE OPTIMIZATION: FLAT CONTIGUOUS LOOKUP TABLES <---
  std::vector<float> window(SAMPLES_PER_FRAME);
  for (int n = 0; n < SAMPLES_PER_FRAME; ++n) {
    window[n] = 0.5f * (1.0f - std::cos(2.0f * M_PI * n / (SAMPLES_PER_FRAME - 1)));
  }

  std::vector<float> cosTable(BARS_COUNT * SAMPLES_PER_FRAME);
  std::vector<float> sinTable(BARS_COUNT * SAMPLES_PER_FRAME);

  for (int k = 0; k < BARS_COUNT; ++k) {
    float freqIndex = std::pow(static_cast<float>(k) / BARS_COUNT, 2.0f) * (SAMPLES_PER_FRAME / 2.0f);
    int offset = k * SAMPLES_PER_FRAME;
    for (int n = 0; n < SAMPLES_PER_FRAME; ++n) {
      float angle = 2.0f * M_PI * freqIndex * n / SAMPLES_PER_FRAME;
      cosTable[offset + n] = std::cos(angle);
      sinTable[offset + n] = std::sin(angle);
    }
  }

  sharedData->fifoFd = open("/tmp/mpd.fifo", O_RDWR | O_NONBLOCK);
  if (sharedData->fifoFd >= 0) {
    // Initial flush: dump any stale data sitting in the pipe
    char discard[4096];
    while (read(sharedData->fifoFd, discard, sizeof(discard)) > 0) {}
  }

  const int bytesPerSample = 2 * 2; 
  int bytesToRead = SAMPLES_PER_FRAME * bytesPerSample;

  // ---> ZERO HEAP ALLOCATIONS PER FRAME <---
  std::vector<int16_t> tempBuffer(SAMPLES_PER_FRAME * 2);
  std::vector<int16_t> finalBuffer(SAMPLES_PER_FRAME * 2);
  std::vector<float> currentSpectrum(BARS_COUNT, 0.0f);
  std::vector<float> monoSamples(SAMPLES_PER_FRAME, 0.0f);

  while (sharedData->running) {
    if (sharedData->fifoFd < 0) {
      sharedData->fifoFd = open("/tmp/mpd.fifo", O_RDWR | O_NONBLOCK);
      if (sharedData->fifoFd < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        continue;
      }
    }

    struct pollfd pfd;
    pfd.fd = sharedData->fifoFd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 50);

    if (ret > 0 && (pfd.revents & POLLIN)) {
      int lastBytesRead = 0;
      bool eofOrError = false;

      // Flush / Drain the pipe completely in one microsecond pass (ncmpcpp style).
      // This dumps any accumulated backlog instantly without running DFT/locks for intermediate chunks.
      while (true) {
        int br = read(sharedData->fifoFd, tempBuffer.data(), bytesToRead);
        if (br > 0) {
          finalBuffer = tempBuffer;
          lastBytesRead = br;
        } else if (br == 0) {
          eofOrError = true;
          break;
        } else {
          if (errno != EAGAIN && errno != EWOULDBLOCK) {
            eofOrError = true;
          }
          break; // Pipe is now completely drained!
        }
      }

      if (eofOrError) {
        close(sharedData->fifoFd);
        sharedData->fifoFd = -1;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        continue;
      }

      if (lastBytesRead > 0) {
        int samplesRead = lastBytesRead / bytesPerSample;
        if (samplesRead > SAMPLES_PER_FRAME) samplesRead = SAMPLES_PER_FRAME;

        for (int i = 0; i < samplesRead; ++i) {
          float left = finalBuffer[i * 2] / 32768.0f;
          float right = finalBuffer[i * 2 + 1] / 32768.0f;
          monoSamples[i] = (left + right) * 0.5f;
        }

        // Fast contiguous array lookups
        for (int k = 0; k < BARS_COUNT; ++k) {
          float re = 0.0f;
          float im = 0.0f;
          int offset = k * SAMPLES_PER_FRAME;
          
          for (int n = 0; n < samplesRead; ++n) {
            float val = monoSamples[n] * window[n];
            re += val * cosTable[offset + n];
            im -= val * sinTable[offset + n];
          }
          
          float magnitude = std::sqrt(re * re + im * im) / (samplesRead * 0.25f);
          currentSpectrum[k] = std::clamp(magnitude * 2.5f, 0.0f, 1.0f);
        }

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
    sharedData->fifoFd = -1;
  }
}

CSharedPointer<IElement> Visualizer::build() {
  m_container =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); }) 
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
      std::chrono::milliseconds(33), // ~30 FPS light update loop
      [weakThis](CAtomicSharedPointer<CTimer>, void *) {
        auto sharedThis = weakThis.lock();
        if (!sharedThis || !sharedThis->m_sharedData->running) return;

        {
          std::lock_guard<std::mutex> lock(sharedThis->m_sharedData->dataMutex);
          sharedThis->m_spectrumBuffer = sharedThis->m_sharedData->smoothedSpectrum;
        }

        bool changed = false;
        if (!sharedThis->m_visualizations.empty()) {
          changed = sharedThis->m_visualizations[sharedThis->m_currentIndex]->update(sharedThis->m_spectrumBuffer);
        }
        
        if (changed) {
          sharedThis->m_container->forceReposition();
        }
        sharedThis->scheduleUpdate();
      },
      nullptr);
}

} // namespace UI::Components

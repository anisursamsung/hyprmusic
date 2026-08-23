#include "DotMatrixVisualization.hpp"
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <algorithm>
#include <cmath>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

const int MATRIX_COLS = 16;
const int MATRIX_ROWS = 10;

CSharedPointer<IElement> DotMatrixVisualization::build(CSharedPointer<CPalette> palette) {
  (void)palette;
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  auto rowLayout = CRowLayoutBuilder::begin()
                       ->gap(8)
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                           CDynamicSize::HT_SIZE_PERCENT,
                                           {1.0F, 1.0F}))
                       ->commence();
  rowLayout->setMargin(25);

  m_dotGrid.clear();
  m_lastColHeights.assign(MATRIX_COLS, 0.0f);

  for (int col = 0; col < MATRIX_COLS; ++col) {
    auto colLayout = CColumnLayoutBuilder::begin()
                         ->gap(6)
                         ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                             CDynamicSize::HT_SIZE_PERCENT,
                                             {1.0f / MATRIX_COLS, 1.0F}))
                         ->commence();

    std::vector<CSharedPointer<CRectangleElement>> columnDots;

    for (int row = MATRIX_ROWS - 1; row >= 0; --row) {
      CHyprColor dimColor(0.15F, 0.15F, 0.22F, 0.20F);

      auto dot = CRectangleBuilder::begin()
                     ->color([dimColor] { return dimColor; })
                     ->rounding(6)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_PERCENT,
                                         {1.0F, 1.0f / MATRIX_ROWS}))
                     ->commence();

      columnDots.push_back(dot);
      colLayout->addChild(dot);
    }

    m_dotGrid.push_back(columnDots);
    rowLayout->addChild(colLayout);
  }

  m_container->addChild(rowLayout);
  return m_container;
}

bool DotMatrixVisualization::update(const std::vector<float>& spectrum) {
  if (spectrum.empty() || m_dotGrid.empty()) return false;

  int binsPerCol = std::max(1, static_cast<int>(spectrum.size()) / MATRIX_COLS);
  bool updated = false;

  for (int col = 0; col < MATRIX_COLS; ++col) {
    float sum = 0.0f;
    for (int j = 0; j < binsPerCol && static_cast<size_t>(col * binsPerCol + j) < spectrum.size(); ++j) {
      sum += spectrum[col * binsPerCol + j];
    }
    float h = std::clamp(sum / binsPerCol, 0.05f, 1.0f);

    if (std::abs(h - m_lastColHeights[col]) > 0.03f) {
      m_lastColHeights[col] = h;
      int activeDots = static_cast<int>(h * MATRIX_ROWS);

      float t = static_cast<float>(col) / static_cast<float>(MATRIX_COLS - 1);
      CHyprColor activeColor(
          0.1f + 0.9f * t,
          0.8f * (1.0f - t) + 0.2f,
          0.9f * std::sin(t * M_PI),
          0.95f
      );

      for (int row = 0; row < MATRIX_ROWS; ++row) {
        size_t listIdx = MATRIX_ROWS - 1 - row;
        if (listIdx >= m_dotGrid[col].size()) continue;

        bool active = (row < activeDots);
        CHyprColor dotCol = active ? activeColor : CHyprColor(0.15F, 0.15F, 0.22F, 0.18F);

        m_dotGrid[col][listIdx]->rebuild()->color([dotCol] { return dotCol; })->commence();
      }
      updated = true;
    }
  }

  return updated;
}

} // namespace UI::Components

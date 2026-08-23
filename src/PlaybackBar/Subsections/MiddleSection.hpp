#pragma once

#include "Common/CustomSeekBar.hpp"
#include <functional>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <memory>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct MiddleSectionContext {
  CSharedPointer<CPalette> palette;
  std::string fontFamily;
  std::function<void(float pct)> onSeek;
};

class MiddleSection {
public:
  explicit MiddleSection(const MiddleSectionContext &ctx);

  CSharedPointer<CRectangleElement> build();
  void updateProgress(unsigned elapsed, unsigned total, bool hasActiveTrack);

private:
  MiddleSectionContext m_ctx;
  std::unique_ptr<CustomSeekBar> m_customSeekBar;
  CSharedPointer<CTextElement> m_timeText;
};

} // namespace UI::Components

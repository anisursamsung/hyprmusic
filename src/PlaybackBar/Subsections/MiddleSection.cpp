#include "MiddleSection.hpp"
#include "Utils/FormatUtils.hpp"
#include <algorithm>

namespace UI::Components {

MiddleSection::MiddleSection(const MiddleSectionContext &ctx) : m_ctx(ctx) {}

CSharedPointer<CRectangleElement> MiddleSection::build() {
  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  auto seekBarSection =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0.0F, 0.0F, 0.0F, 0.0F); })
          ->rounding(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto seekBarRow =
      CRowLayoutBuilder::begin()
          ->gap(12)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  seekBarRow->setMargin(6);

  m_timeText =
      CTextBuilder::begin()
          ->text(std::string("0:00 / 0:00"))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.8, 0.8, 0.8, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->align(HT_FONT_ALIGN_LEFT)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_timeText->setGrow(false);

  auto onSeekCb = m_ctx.onSeek;
  CustomSeekBar::Context seekCtx{
      .palette = palette,
      .onSeek =
          [onSeekCb](float pct) {
            if (onSeekCb) {
              onSeekCb(pct);
            }
          },
      .size = CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                           CDynamicSize::HT_SIZE_ABSOLUTE, {0.0F, 40.0F})};

  m_customSeekBar = std::make_unique<CustomSeekBar>(seekCtx);
  auto seekBarElem = m_customSeekBar->build();
  seekBarElem->setGrow(true);

  seekBarRow->addChild(seekBarElem);
  seekBarRow->addChild(m_timeText);
  seekBarSection->addChild(seekBarRow);

  return seekBarSection;
}

void MiddleSection::updateProgress(unsigned elapsed, unsigned total,
                                   bool hasActiveTrack) {
  if (m_timeText) {
    std::string timeStr = "0:00 / 0:00";
    if (hasActiveTrack && total > 0) {
      timeStr = Utils::formatTime(elapsed) + " / " + Utils::formatTime(total);
    }
    m_timeText->rebuild()->text(std::string(timeStr))->commence();
  }

  if (m_customSeekBar) {
    float progress = 0.0f;
    if (hasActiveTrack && total > 0) {
      progress = std::clamp(
          static_cast<float>(elapsed) / static_cast<float>(total), 0.0f, 1.0f);
    }
    m_customSeekBar->updateProgress(progress, hasActiveTrack);
  }
}

} // namespace UI::Components

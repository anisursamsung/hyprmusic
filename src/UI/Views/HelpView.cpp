#include "HelpView.hpp"
#include <hyprtoolkit/element/Line.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <fstream>
#include <vector>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

HelpView::HelpView(const HelpViewContext &ctx) : m_ctx(ctx) {}

void HelpView::rebuildUI(CSharedPointer<CRectangleElement> wrapper) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  m_tabContentWrapper->clearChildren();

  auto helpMainLayout =
      CColumnLayoutBuilder::begin()
          ->gap(15)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  helpMainLayout->setMargin(20);
  m_tabContentWrapper->addChild(helpMainLayout);

  auto scrollArea =
      CScrollAreaBuilder::begin()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->scrollY(true)
          ->commence();
  helpMainLayout->addChild(scrollArea);

  m_helpContentLayout =
      CColumnLayoutBuilder::begin()
          ->gap(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_helpContentLayout->setMargin(10);
  scrollArea->addChild(m_helpContentLayout);

  std::string helpFilePath = "HELP.md";
  std::ifstream inFile(helpFilePath);
  std::vector<std::string> lines;
  if (inFile.is_open()) {
    std::string line;
    while (std::getline(inFile, line)) {
      lines.push_back(line);
    }
    inFile.close();
  }

  if (lines.empty()) {
    lines = {
        "# HyprMusic - User Guide & Overview",
        "HyprMusic is a native Linux music player designed for Hyprland using Hyprtoolkit.",
        "---",
        "## Key Features & Tabs",
        "### 1. Queue Tab",
        "- View active playback queue, search tracks, shuffle, and clear queue.",
        "### 2. Database Tab",
        "- Browse local music folder structure and search music database.",
        "### 3. Playlists Tab",
        "- Manage saved MPD playlists and view playlist tracks.",
        "### 4. YT-DLP Online Search Tab",
        "- Search YouTube music audio and stream directly into MPD.",
        "### 5. Settings Tab",
        "- Configure MPD paths, audio outputs, and server options.",
        "---",
        "## Playback & Control Bar",
        "- Interactive bottom bar with track details, seek slider, play/pause, and volume."};
  }

  for (const auto &rawLine : lines) {
    std::string line = rawLine;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line.rfind("# ", 0) == 0) {
      auto titleTxt =
          CTextBuilder::begin()
              ->text(std::string(line.substr(2)))
              ->color([palette] {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
              ->align(HT_FONT_ALIGN_LEFT)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_helpContentLayout->addChild(titleTxt);
    } else if (line.rfind("## ", 0) == 0) {
      auto titleTxt =
          CTextBuilder::begin()
              ->text(std::string(line.substr(3)))
              ->color([palette] {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_H2))
              ->align(HT_FONT_ALIGN_LEFT)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_helpContentLayout->addChild(titleTxt);
    } else if (line.rfind("### ", 0) == 0) {
      auto titleTxt =
          CTextBuilder::begin()
              ->text(std::string(line.substr(4)))
              ->color([palette] {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
              ->align(HT_FONT_ALIGN_LEFT)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_helpContentLayout->addChild(titleTxt);
    } else if (line == "---") {
      auto lineDivider =
          CLineBuilder::begin()
              ->color([palette] {
                return palette ? palette->m_colors.alternateBase
                               : CHyprColor(0.3, 0.3, 0.3, 0.5);
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 2.0F}))
              ->commence();
      lineDivider->setGrow(false);
      m_helpContentLayout->addChild(lineDivider);
    } else if (!line.empty()) {
      auto bodyTxt =
          CTextBuilder::begin()
              ->text(std::string(line))
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(0.9, 0.9, 0.9, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->align(HT_FONT_ALIGN_LEFT)
              ->noEllipsize(false)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      m_helpContentLayout->addChild(bodyTxt);
    }
  }

  m_tabContentWrapper->forceReposition();
}

} // namespace UI::Views

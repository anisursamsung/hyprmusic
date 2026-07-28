#include "HelpView.hpp"
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
          ->gap(12)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_helpContentLayout->setMargin(5);
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

  CSharedPointer<CColumnLayoutElement> currentCardLayout = nullptr;

  auto finalizeCard = [this, palette](CSharedPointer<CColumnLayoutElement> cardLayout) {
    if (!cardLayout)
      return;
    auto cardBg =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.base
                             : CHyprColor(0.15, 0.15, 0.15, 1.0);
            })
            ->rounding(palette ? palette->m_vars.smallRounding : 5)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    cardBg->addChild(cardLayout);
    m_helpContentLayout->addChild(cardBg);
  };

  for (const auto &rawLine : lines) {
    std::string line = rawLine;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    if (line.rfind("# ", 0) == 0) {
      if (currentCardLayout) {
        finalizeCard(currentCardLayout);
        currentCardLayout = nullptr;
      }

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
      if (currentCardLayout) {
        finalizeCard(currentCardLayout);
        currentCardLayout = nullptr;
      }

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
      if (currentCardLayout) {
        finalizeCard(currentCardLayout);
        currentCardLayout = nullptr;
      }

      currentCardLayout =
          CColumnLayoutBuilder::begin()
              ->gap(8)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      currentCardLayout->setMargin(12);

      auto h3Txt =
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
      currentCardLayout->addChild(h3Txt);
    } else if (!line.empty() && line != "---") {
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
      if (currentCardLayout) {
        currentCardLayout->addChild(bodyTxt);
      } else {
        m_helpContentLayout->addChild(bodyTxt);
      }
    }
  }

  if (currentCardLayout) {
    finalizeCard(currentCardLayout);
  }

  m_tabContentWrapper->forceReposition();
}

} // namespace UI::Views

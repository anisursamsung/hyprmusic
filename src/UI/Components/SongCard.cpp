#include "SongCard.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/core/Input.hpp>

namespace UI::Components {

SongCard::SongCard(const SongCardConfig &cfg) : m_cfg(cfg), m_active(cfg.isActive) {}

void SongCard::setTitle(const std::string &title) {
  m_cfg.title = title;
  if (m_titleText) {
    m_titleText->rebuild()
        ->text(std::string(title))
        ->commence();
  }
}

void SongCard::setSubtitle(const std::string &subtitle) {
  m_cfg.subtitle = subtitle;
  if (m_subtitleText) {
    m_subtitleText->rebuild()
        ->text(std::string(subtitle))
        ->commence();
  }
}

void SongCard::setImagePath(const std::string &path) {
  m_cfg.imagePath = path;
  if (!m_artContainer)
    return;
  std::string defaultPath = Utils::getDefaultArtworkPath();
  if (path.empty() || path == defaultPath)
    return;

  m_artContainer->clearChildren();
  m_artImage = CImageBuilder::begin()
                   ->path(std::string(path))
                   ->fitMode(IMAGE_FIT_MODE_COVER)
                   ->rounding(6)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                       CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                   ->commence();
  m_artContainer->addChild(m_artImage);
}

void SongCard::setActive(bool active) {
  m_active = active;
  if (m_card) {
    m_card->rebuild()
        ->color([this] {
          if (m_active) {
            auto palette = m_cfg.palette;
            auto c = palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
            return CHyprColor(c.r, c.g, c.b, 0.12);
          }
          return CHyprColor(0, 0, 0, 0);
        })
        ->borderColor([this] {
          auto palette = m_cfg.palette;
          if (m_active) {
            return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
          }
          auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
          return CHyprColor(c.r, c.g, c.b, 0.15);
        })
        ->borderThickness(m_active ? 1 : 0)
        ->commence();
  }
}

// ── build() ───────────────────────────────────────────────────────────────

CSharedPointer<CRectangleElement> SongCard::build() {
  auto palette    = m_cfg.palette;
  auto fontFamily = m_cfg.fontFamily;

  // ── Card container ────────────────────────────────────────────────────
  m_card =
      CRectangleBuilder::begin()
          ->color([this] {
            if (m_active) {
              auto palette = m_cfg.palette;
              auto c = palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
              return CHyprColor(c.r, c.g, c.b, 0.12);
            }
            return CHyprColor(0, 0, 0, 0);
          })
          ->borderColor([this] {
            auto palette = m_cfg.palette;
            if (m_active) {
              return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
            }
            auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            return CHyprColor(c.r, c.g, c.b, 0.15);
          })
          ->borderThickness(m_active ? 1 : 0)
          ->rounding(m_cfg.rounding)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE,
                              {1.0F, m_cfg.cardHeight}))
          ->commence();

  if (m_cfg.onCardBodyClick) {
    auto cb = m_cfg.onCardBodyClick;
    m_card->setReceivesMouse(true);
    m_card->setMouseButton([cb](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down)
        cb();
    });
  }

  // ── Outer row: [artContainer] [textCol] [actionBtn] ───────────────────
  auto rowLayout =
      CRowLayoutBuilder::begin()
          ->gap(14)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  rowLayout->setMargin(10);

  // ── Artwork container ────────────────────────────────────────────────
  float artSize = m_cfg.cardHeight > 20.0f ? m_cfg.cardHeight - 20.0f : 48.0f;
  m_artContainer =
      CRectangleBuilder::begin()
          ->color([palette] {
            auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            return CHyprColor(c.r, c.g, c.b, 0.08);
          })
          ->rounding(6)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {artSize, artSize}))
          ->commence();
  m_artContainer->setGrow(false);

  if (m_cfg.onCardBodyClick) {
    auto cb = m_cfg.onCardBodyClick;
    m_artContainer->setReceivesMouse(true);
    m_artContainer->setMouseButton([cb](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down)
        cb();
    });
  }

  std::string defaultPath = Utils::getDefaultArtworkPath();
  if (!m_cfg.imagePath.empty() && m_cfg.imagePath != defaultPath) {
    m_artImage = CImageBuilder::begin()
                     ->path(std::string(m_cfg.imagePath))
                     ->fitMode(IMAGE_FIT_MODE_COVER)
                     ->rounding(6)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                     ->commence();
    m_artContainer->addChild(m_artImage);
  } else {
    auto noteIcon = CTextBuilder::begin()
                        ->text("🎵")
                        ->color([palette] {
                          auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
                          return CHyprColor(c.r, c.g, c.b, 0.35);
                        })
                        ->fontFamily(std::string(fontFamily))
                        ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                        ->align(HT_FONT_ALIGN_CENTER)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                        ->commence();
    noteIcon->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    noteIcon->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    m_artContainer->addChild(noteIcon);
  }
  rowLayout->addChild(m_artContainer);

  // ── Action button (⋮) — built here, added to row after textCol ──────────
  auto actionCb = m_cfg.onActionClick;
  auto actionBtn =
      CButtonBuilder::begin()
          ->label("⌄")
          ->alignText(HT_FONT_ALIGN_CENTER)
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H2))
          ->noBg(true)
          ->noBorder(true)
          ->onMainClick([actionCb](CSharedPointer<CButtonElement>) {
            if (actionCb)
              actionCb();
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {36.0F, 44.0F}))
          ->commence();
  actionBtn->setGrow(false);

  // ── Title text ────────────────────────────────────────────────────────
  m_titleText =
      CTextBuilder::begin()
          ->text(std::string(m_cfg.title))
          ->color([palette] {
            return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->align(HT_FONT_ALIGN_LEFT)
          ->noEllipsize(false)
          ->interactable(true)
          ->commence();

  if (m_cfg.onCardBodyClick) {
    auto cb = m_cfg.onCardBodyClick;
    m_titleText->setReceivesMouse(true);
    m_titleText->setMouseButton([cb](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down)
        cb();
    });
  }

  // ── Subtitle (artist) text ────────────────────────────────────────────
  m_subtitleText =
      CTextBuilder::begin()
          ->text(std::string(m_cfg.subtitle))
          ->color([palette] {
            auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            return CHyprColor(c.r, c.g, c.b, 0.55);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL))
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->align(HT_FONT_ALIGN_LEFT)
          ->noEllipsize(false)
          ->commence();

  // ── titleRow — bounding box for title ellipsis ────────────────────────
  auto titleRow =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  titleRow->addChild(m_titleText);

  // ── subtitleRow — bounding box for subtitle ellipsis ─────────────────
  auto subtitleRow =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  subtitleRow->addChild(m_subtitleText);

  // ── textCol — grows horizontally to fill remaining space ──────────────
  auto textCol =
      CColumnLayoutBuilder::begin()
          ->gap(4)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  textCol->setGrow(true);
  textCol->addChild(titleRow);
  textCol->addChild(subtitleRow);

  rowLayout->addChild(textCol);
  rowLayout->addChild(actionBtn);  // ⋮ button on the right
  m_card->addChild(rowLayout);

  return m_card;
}

} // namespace UI::Components

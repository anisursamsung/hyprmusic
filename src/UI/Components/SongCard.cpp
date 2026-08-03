#include "SongCard.hpp"
#include <hyprtoolkit/core/Input.hpp>

namespace UI::Components {

SongCard::SongCard(const SongCardConfig &cfg) : m_cfg(cfg), m_active(cfg.isActive) {}

// ── Private colour helpers ─────────────────────────────────────────────────

void SongCard::applyTitleColor() {
  if (!m_titleText)
    return;
  auto palette = m_cfg.palette;
  bool active  = m_active;
  m_titleText->rebuild()
      ->color([palette, active] {
        if (active)
          return palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
        return palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
      })
      ->commence();
}

void SongCard::applySubtitleColor() {
  if (!m_subtitleText)
    return;
  auto palette = m_cfg.palette;
  bool active  = m_active;
  m_subtitleText->rebuild()
      ->color([palette, active] {
        if (active) {
          auto c = palette ? palette->m_colors.accent : CHyprColor(0.2, 0.8, 0.4, 1.0);
          return CHyprColor(c.r, c.g, c.b, 0.7);
        }
        auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
        return CHyprColor(c.r, c.g, c.b, 0.55);
      })
      ->commence();
}

// ── Live setters ──────────────────────────────────────────────────────────

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

void SongCard::setActive(bool active) {
  m_active = active;
  applyTitleColor();
  applySubtitleColor();
}

// ── build() ───────────────────────────────────────────────────────────────

CSharedPointer<CRectangleElement> SongCard::build() {
  auto palette    = m_cfg.palette;
  auto fontFamily = m_cfg.fontFamily;

  // ── Card container ────────────────────────────────────────────────────
  m_card =
      CRectangleBuilder::begin()
          ->color([] { return CHyprColor(0, 0, 0, 0); })
          ->borderColor([palette] {
            auto c = palette ? palette->m_colors.text : CHyprColor(1, 1, 1, 1);
            return CHyprColor(c.r, c.g, c.b, 0.15);
          })
          ->borderThickness(1)
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

  // ── Outer row: [actionBtn] [textCol] ─────────────────────────────────
  auto rowLayout =
      CRowLayoutBuilder::begin()
          ->gap(14)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  rowLayout->setMargin(10);

  // ── Action button (⋮) — built here, added to row after textCol ──────────
  auto actionCb = m_cfg.onActionClick;
  auto actionBtn =
      CButtonBuilder::begin()
          ->label("⋮")
          ->alignText(HT_FONT_ALIGN_CENTER)
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->onMainClick([actionCb](CSharedPointer<CButtonElement>) {
            if (actionCb)
              actionCb();
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {32.0F, 32.0F}))
          ->commence();
  actionBtn->setGrow(false);

  // ── Title text ────────────────────────────────────────────────────────
  auto palette_title = palette;
  bool active_title  = m_active;
  m_titleText =
      CTextBuilder::begin()
          ->text(std::string(m_cfg.title))
          ->color([palette_title, active_title] {
            if (active_title)
              return palette_title ? palette_title->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
            return palette_title ? palette_title->m_colors.text
                                 : CHyprColor(1, 1, 1, 1);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
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
  auto palette_sub = palette;
  bool active_sub  = m_active;
  m_subtitleText =
      CTextBuilder::begin()
          ->text(std::string(m_cfg.subtitle))
          ->color([palette_sub, active_sub] {
            if (active_sub) {
              auto c = palette_sub ? palette_sub->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
              return CHyprColor(c.r, c.g, c.b, 0.7);
            }
            auto c = palette_sub ? palette_sub->m_colors.text
                                 : CHyprColor(1, 1, 1, 1);
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

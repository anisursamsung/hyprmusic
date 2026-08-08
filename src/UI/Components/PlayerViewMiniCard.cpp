#include "PlayerViewMiniCard.hpp"
#include "../../Utils/FormatUtils.hpp"

namespace UI::Components {

PlayerViewMiniCard::PlayerViewMiniCard(const PlayerViewMiniCardConfig &cfg) : m_cfg(cfg) {}

CSharedPointer<CRectangleElement> PlayerViewMiniCard::build() {
  auto palette = m_cfg.palette;
  std::string fontFamily = m_cfg.fontFamily;

  // Root Card: 80% width and 80% height of parent
  m_root = CRectangleBuilder::begin()
               ->color([palette] {
                 if (!palette) return CHyprColor(0.08F, 0.08F, 0.12F, 0.85F);
                 return palette->m_colors.base.mix(CHyprColor(0.0F, 0.0F, 0.0F, 0.85F), 0.6F);
               })
               ->rounding(24)
               ->borderThickness(1)
               ->borderColor([palette] {
                 if (!palette) return CHyprColor(0.3F, 0.3F, 0.35F, 0.5F);
                 return palette->m_colors.alternateBase.mix(palette->m_colors.text, 0.2F);
               })
               ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.80F, 0.80F}))
               ->commence();

  // Horizontal Row splitting into Left Zone (Artwork) & Right Zone (Details)
  auto mainRow = CRowLayoutBuilder::begin()
                     ->gap(24)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                     ->commence();
  mainRow->setMargin(28);

  // 1. LEFT ZONE: Artwork Only (48% width)
  auto leftZone = CRectangleBuilder::begin()
                      ->color([] { return CHyprColor(0, 0, 0, 0); })
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.48F, 1.0F}))
                      ->commence();

  if (!m_cfg.artPath.empty()) {
    m_coverImage = CImageBuilder::begin()
                       ->path(std::string(m_cfg.artPath))
                       ->fitMode(IMAGE_FIT_MODE_CONTAIN)
                       ->rounding(16)
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                       ->commence();
    m_coverImage->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    m_coverImage->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    leftZone->addChild(m_coverImage);
  }
  mainRow->addChild(leftZone);

  // 2. RIGHT ZONE: Song Details Bounding Box Wrapper (48% width)
  auto rightZone = CRectangleBuilder::begin()
                       ->color([] { return CHyprColor(0, 0, 0, 0); })
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.48F, 1.0F}))
                       ->commence();

  auto rightColumn = CColumnLayoutBuilder::begin()
                         ->gap(16)
                         ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                         ->commence();
  rightColumn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
  rightColumn->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);

  // Track Title Bounding Box Row
  auto titleRow = CRowLayoutBuilder::begin()
                      ->gap(0)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                      ->commence();

  std::string titleStr = m_cfg.title.empty() ? "No Song Playing" : m_cfg.title;
  m_titleText = CTextBuilder::begin()
                    ->text(Utils::wrapText(titleStr, 22))
                    ->color([palette] {
                      return palette ? palette->m_colors.text : CHyprColor(1.0F, 1.0F, 1.0F, 1.0F);
                    })
                    ->fontFamily(std::string(fontFamily))
                    ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
                    ->align(HT_FONT_ALIGN_LEFT)
                    ->noEllipsize(true)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                    ->commence();
  titleRow->addChild(m_titleText);
  rightColumn->addChild(titleRow);

  // Artist Name Bounding Box Row
  auto artistRow = CRowLayoutBuilder::begin()
                       ->gap(0)
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                       ->commence();

  std::string artistStr = m_cfg.artist.empty() ? "Unknown Artist" : m_cfg.artist;
  m_artistText = CTextBuilder::begin()
                     ->text(Utils::wrapText(artistStr, 26))
                     ->color([palette] {
                       return palette ? palette->m_colors.accent : CHyprColor(0.4F, 0.8F, 1.0F, 1.0F);
                     })
                     ->fontFamily(std::string(fontFamily))
                     ->fontSize(CFontSize(CFontSize::HT_FONT_H2))
                     ->align(HT_FONT_ALIGN_LEFT)
                     ->noEllipsize(true)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                     ->commence();
  artistRow->addChild(m_artistText);
  rightColumn->addChild(artistRow);

  // Time Display Bounding Box Row
  auto timeRow = CRowLayoutBuilder::begin()
                     ->gap(0)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                     ->commence();

  std::string timeStr = m_cfg.timeStr.empty() ? "0:00" : m_cfg.timeStr;
  m_timeText = CTextBuilder::begin()
                   ->text(std::string(timeStr))
                   ->color([palette] {
                     return palette ? palette->m_colors.text.mix(palette->m_colors.base, 0.35F)
                                    : CHyprColor(0.75F, 0.75F, 0.75F, 1.0F);
                   })
                   ->fontFamily(std::string(fontFamily))
                   ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                   ->align(HT_FONT_ALIGN_LEFT)
                   ->noEllipsize(true)
                   ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                   ->commence();
  timeRow->addChild(m_timeText);
  rightColumn->addChild(timeRow);

  rightZone->addChild(rightColumn);
  mainRow->addChild(rightZone);

  m_root->addChild(mainRow);
  return m_root;
}

void PlayerViewMiniCard::updateInfo(const std::string &title, const std::string &artist, const std::string &timeStr) {
  m_cfg.title = title;
  m_cfg.artist = artist;
  m_cfg.timeStr = timeStr;

  std::string formattedTitle = Utils::wrapText(title.empty() ? "No Song Playing" : title, 22);
  std::string formattedArtist = Utils::wrapText(artist.empty() ? "Unknown Artist" : artist, 26);
  std::string formattedTime = timeStr.empty() ? "0:00" : timeStr;

  if (m_titleText) {
    m_titleText->rebuild()
        ->text(std::move(formattedTitle))
        ->noEllipsize(true)
        ->commence();
  }
  if (m_artistText) {
    m_artistText->rebuild()
        ->text(std::move(formattedArtist))
        ->noEllipsize(true)
        ->commence();
  }
  if (m_timeText) {
    m_timeText->rebuild()
        ->text(std::move(formattedTime))
        ->noEllipsize(true)
        ->commence();
  }
}

void PlayerViewMiniCard::updateArt(const std::string &artPath) {
  m_cfg.artPath = artPath;
  if (m_coverImage) {
    m_coverImage->rebuild()->path(std::string(artPath))->commence();
  }
}

} // namespace UI::Components

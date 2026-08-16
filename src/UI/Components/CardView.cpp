/*
 * CardView Layout Nesting Hierarchy:
 *
 * m_root (CRectangleElement)
 *  └── cardColumn (CColumnLayoutElement)
 *       ├── artContainer (CRectangleElement)
 *       │    └── m_albumArt (CImageElement)
 *       │
 *       └── textContainer (CRectangleElement)
 *            └── textColumn (CColumnLayoutElement)
 *                 ├── m_titleText (CTextElement)
 *                 └── m_subtitleText (CTextElement)
 *
 * Flow:
 * - Root Container (m_root) contains cardColumn.
 * - cardColumn stacks artContainer on top and textContainer on the bottom.
 * - artContainer wraps m_albumArt.
 * - textContainer wraps textColumn (which holds m_titleText and m_subtitleText).
 */

#include "CardView.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>

namespace UI::Components {

CardView::CardView(const CardViewConfig &cfg) : m_cfg(cfg) {}

CSharedPointer<CRectangleElement> CardView::build() {
    auto palette = m_cfg.palette;
    std::string fontFamily = m_cfg.fontFamily;

    m_root = CRectangleBuilder::begin()
        ->color([palette] {
            if (!palette) return CHyprColor(0.16, 0.16, 0.20, 1.0);
            return palette->m_colors.alternateBase.mix(palette->m_colors.base, 0.35);
        })
        ->rounding(10)
        ->borderThickness(1)
        ->borderColor([palette] {
            if (!palette) return CHyprColor(0.3, 0.3, 0.3, 0.5);
            return palette->m_colors.text.mix(palette->m_colors.alternateBase, 0.85);
        })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();

    if (m_cfg.onClick) {
        m_root->setReceivesMouse(true);
        m_root->setMouseButton([this](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down && m_cfg.onClick) {
                m_cfg.onClick();
            }
        });
    }

    auto cardColumn = CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();

    // 1. Artwork Area (ABSOLUTE height 1.0F with setGrow(true) to fill remaining vertical space)
    m_artContainer = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
        ->commence();
    m_artContainer->setGrow(true);

    std::string artPath = m_cfg.imagePath.empty() ? Utils::getDefaultArtworkPath() : m_cfg.imagePath;

    m_albumArt = CImageBuilder::begin()
        ->path(std::string(artPath))
        ->fitMode(IMAGE_FIT_MODE_COVER)
        ->rounding(8)
        ->sync(true)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.90F, 0.90F}))
        ->commence();
    m_albumArt->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    m_albumArt->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    m_artContainer->addChild(m_albumArt);
    cardColumn->addChild(m_artContainer);

    // 2. Song Title & Subtitle Section Wrapper
    std::string titleStr = m_cfg.title.empty() ? (m_cfg.text.empty() ? "No currently playing songs" : m_cfg.text) : m_cfg.title;
    std::string subStr = m_cfg.subtitle;

    m_titleText =
        CTextBuilder::begin()
            ->text(std::string(titleStr))
            ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_CENTER)
            ->noEllipsize(false)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    m_subtitleText =
        CTextBuilder::begin()
            ->text(std::string(subStr))
            ->color([palette] {
                if (!palette) return CHyprColor(0.7, 0.7, 0.7, 1.0);
                return palette->m_colors.text.mix(palette->m_colors.alternateBase, 0.4);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL))
            ->align(HT_FONT_ALIGN_CENTER)
            ->noEllipsize(false)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    auto textColumn =
        CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    textColumn->setMargin(4);
    textColumn->addChild(m_titleText);
    textColumn->addChild(m_subtitleText);

    auto textContainer = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 0.35F}))
        ->commence();

    textContainer->addChild(textColumn);
    cardColumn->addChild(textContainer);

    m_root->addChild(cardColumn);
    m_root->forceReposition();
    return m_root;
}

void CardView::updateImage(const std::string &imagePath) {
    m_cfg.imagePath = imagePath;
    if (m_artContainer) {
        m_artContainer->clearChildren();
        std::string artPath = imagePath.empty() ? Utils::getDefaultArtworkPath() : imagePath;
        m_albumArt = CImageBuilder::begin()
            ->path(std::string(artPath))
            ->fitMode(IMAGE_FIT_MODE_COVER)
            ->rounding(8)
            ->sync(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.90F, 0.90F}))
            ->commence();
        m_albumArt->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        m_albumArt->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
        m_artContainer->addChild(m_albumArt);
        if (m_root) {
            m_root->forceReposition();
        }
    }
}

void CardView::updateInfo(const std::string &title, const std::string &subtitle) {
    m_cfg.title = title;
    m_cfg.subtitle = subtitle;
    std::string titleStr = title.empty() ? "No currently playing songs" : title;
    if (m_titleText) {
        m_titleText->rebuild()
            ->text(std::string(titleStr))
            ->commence();
    }
    if (m_subtitleText) {
        m_subtitleText->rebuild()
            ->text(std::string(subtitle))
            ->commence();
    }
}

void CardView::updateText(const std::string &text) {
    updateInfo(text, "");
}

void CardView::setOnClick(std::function<void()> onClick) {
    m_cfg.onClick = onClick;
    if (m_root) {
        m_root->setReceivesMouse(true);
        m_root->setMouseButton([this](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down && m_cfg.onClick) {
                m_cfg.onClick();
            }
        });
    }
}

} // namespace UI::Components

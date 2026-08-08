#include "CardView.hpp"
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

    auto leftColumn = CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
        ->commence();

    // 1. Artwork Area (ABSOLUTE height 1.0F with setGrow(true) to fill remaining vertical space)
    auto artArea = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
        ->commence();
    artArea->setGrow(true);

    if (!m_cfg.imagePath.empty()) {
        m_albumArt = CImageBuilder::begin()
            ->path(std::string(m_cfg.imagePath))
            ->fitMode(IMAGE_FIT_MODE_COVER)
            ->rounding(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.90F, 0.90F}))
            ->commence();
        m_albumArt->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
        m_albumArt->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
        artArea->addChild(m_albumArt);
    }
    leftColumn->addChild(artArea);

    // 2. Song Title Section Wrapper (takes auto height + 4px margin)
    std::string textStr = m_cfg.text.empty() ? "No currently playing songs" : m_cfg.text;
    m_titleText =
        CTextBuilder::begin()
            ->text(std::string(textStr))
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

    auto titleRow =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    titleRow->setMargin(4);
    titleRow->addChild(m_titleText);

    auto titleContainer = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
        ->commence();

    titleContainer->addChild(titleRow);
    leftColumn->addChild(titleContainer);

    m_root->addChild(leftColumn);
    return m_root;
}

void CardView::updateImage(const std::string &imagePath) {
    m_cfg.imagePath = imagePath;
    if (m_albumArt) {
        m_albumArt->rebuild()
            ->path(std::string(imagePath))
            ->commence();
    }
}

void CardView::updateText(const std::string &text) {
    m_cfg.text = text;
    std::string textStr = text.empty() ? "No currently playing songs" : text;
    if (m_titleText) {
        m_titleText->rebuild()
            ->text(std::string(textStr))
            ->commence();
    }
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

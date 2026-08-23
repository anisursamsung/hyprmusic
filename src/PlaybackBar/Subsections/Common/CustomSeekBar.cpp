#include "CustomSeekBar.hpp"
#include <algorithm>
namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

CustomSeekBar::CustomSeekBar(const Context &ctx) : m_ctx(ctx) {}

CSharedPointer<IElement> CustomSeekBar::build() {
    auto palette = m_ctx.palette;
    int rounding = m_ctx.rounding >= 0 ? m_ctx.rounding : (palette ? palette->m_vars.smallRounding : 5);

    // 1. Outer container track
    m_container = CButtonBuilder::begin()
         ->label("")
         ->noBg(true)
         ->noBorder(true)
         ->size(std::move(m_ctx.size))
         ->commence();
    // 2. Inner fill progress bar
    m_fillBar = CRectangleBuilder::begin()
        ->color([palette] {
            return palette ? palette->m_colors.text : CHyprColor(1.0, 1.0, 1.0, 1.0);
        })
        ->rounding(rounding)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.0F, 1.0F}))
        ->commence();

    m_container->addChild(m_fillBar);
    m_container->setReceivesMouse(true);

    // Track the last known local X coordinate within the element
    auto lastLocalX = std::make_shared<double>(0.0);

    // Lambda to update UI and trigger seek callback
    auto updateValueFromLocalX = [this](double localX) {
        auto size = m_container->size();
        if (size.x <= 0.0) return;

        float pct = std::clamp(static_cast<float>(localX / size.x), 0.0f, 1.0f);
        m_currentProgress = pct;

        if (m_fillBar) {
            auto palette = m_ctx.palette;
            int rounding = m_ctx.rounding >= 0 ? m_ctx.rounding : (palette ? palette->m_vars.smallRounding : 5);
            m_fillBar = m_fillBar->rebuild()
                ->color([palette] { return palette ? palette->m_colors.text : CHyprColor(1.0, 1.0, 1.0, 1.0); })
                ->rounding(rounding)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {pct, 1.0F}))
                ->commence();
            m_container->forceReposition();
        }

        if (m_ctx.onSeek) {
            m_ctx.onSeek(pct);
        }
    };

    // Cache local X position whenever the mouse moves over the element
    m_container->setMouseMove([this, lastLocalX, updateValueFromLocalX](const Hyprutils::Math::Vector2D &pos) {
        *lastLocalX = pos.x;
        if (m_isDragging) {
            updateValueFromLocalX(pos.x);
        }
    });

    // Handle single clicks and drag state changes using the cached local X
    m_container->setMouseButton([this, lastLocalX, updateValueFromLocalX](Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        m_isDragging = down;
        if (down) {
            // Instantly seek to where the user single-tapped
            updateValueFromLocalX(*lastLocalX);
        }
    });

    return m_container;
}

void CustomSeekBar::updateProgress(float progress, bool /*isPlaying*/) {
    if (m_isDragging) return; 
    m_currentProgress = std::clamp(progress, 0.0f, 1.0f);

    if (m_fillBar && m_container) {
        auto palette = m_ctx.palette;
        int rounding = m_ctx.rounding >= 0 ? m_ctx.rounding : (palette ? palette->m_vars.smallRounding : 5);
        m_fillBar = m_fillBar->rebuild()
            ->color([palette] { return palette ? palette->m_colors.text : CHyprColor(1.0, 1.0, 1.0, 1.0); })
            ->rounding(rounding)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {m_currentProgress, 1.0F}))
            ->commence();
        m_container->forceReposition();
    }
}

} // namespace UI::Components

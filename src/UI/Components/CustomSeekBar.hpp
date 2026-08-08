#pragma once

#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <functional>
#include <memory>
#include <hyprtoolkit/element/Button.hpp>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

class CustomSeekBar {
public:
    struct Context {
        CSharedPointer<IWindow> window;
        CSharedPointer<CPalette> palette;
        std::function<void(float)> onSeek;
	CDynamicSize size = CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {20.0F, 44.0F});
        int rounding = -1; // -1 uses default palette smallRounding
    };

    explicit CustomSeekBar(const Context &ctx);

    CSharedPointer<IElement> build();
    void updateProgress(float progress, bool isPlaying);

    bool isSliding() const { return m_isDragging; }

private:
    Context m_ctx;
    CSharedPointer<CButtonElement> m_container;
    CSharedPointer<CRectangleElement> m_fillBar;
    
    bool m_isDragging = false;
    float m_currentProgress = 0.0f;

    void handleSeekInput(double localX, double totalWidth);
};

} // namespace UI::Components

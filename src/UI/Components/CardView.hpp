#pragma once

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <functional>
#include <memory>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct CardViewConfig {
    CSharedPointer<CPalette> palette;
    std::string fontFamily = "Sans Serif";
    std::string imagePath;
    std::string text = "No currently playing songs";
    std::function<void()> onClick;
};

class CardView {
public:
    explicit CardView(const CardViewConfig &cfg);

    CSharedPointer<CRectangleElement> build();

    void updateImage(const std::string &imagePath);
    void updateText(const std::string &text);
    void setOnClick(std::function<void()> onClick);

    CSharedPointer<CRectangleElement> root() const { return m_root; }

private:
    CardViewConfig m_cfg;

    CSharedPointer<CRectangleElement> m_root;
    CSharedPointer<CImageElement> m_albumArt;
    CSharedPointer<CTextElement> m_titleText;
};

} // namespace UI::Components

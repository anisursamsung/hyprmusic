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

struct MiniArtworkCardConfig {
    CSharedPointer<CPalette> palette;
    std::string fontFamily = "Sans Serif";
    std::string imagePath;
    std::string title = "No currently playing songs";
    std::string subtitle;
    std::function<void()> onClick;
};

class MiniArtworkCard {
public:
    explicit MiniArtworkCard(const MiniArtworkCardConfig &cfg);

    CSharedPointer<CRectangleElement> build();

    void updateImage(const std::string &imagePath);
    void updateInfo(const std::string &title, const std::string &subtitle);

private:
    MiniArtworkCardConfig m_cfg;

    CSharedPointer<CRectangleElement> m_root;
    CSharedPointer<CRectangleElement> m_artContainer;
    CSharedPointer<CImageElement> m_albumArt;
    CSharedPointer<CTextElement> m_titleText;
    CSharedPointer<CTextElement> m_subtitleText;
};

} // namespace UI::Components

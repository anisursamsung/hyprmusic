#pragma once

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <memory>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlayerViewMiniCardConfig {
  CSharedPointer<CPalette> palette;
  std::string fontFamily = "Sans Serif";
  std::string title = "No Song Playing";
  std::string artist = "Unknown Artist";
  std::string artPath;
  std::string timeStr = "0:00";
};

class PlayerViewMiniCard {
public:
  explicit PlayerViewMiniCard(const PlayerViewMiniCardConfig &cfg);

  CSharedPointer<CRectangleElement> build();

  void updateInfo(const std::string &title, const std::string &artist, const std::string &timeStr);
  void updateArt(const std::string &artPath);

  CSharedPointer<CRectangleElement> root() const { return m_root; }

private:
  PlayerViewMiniCardConfig m_cfg;

  CSharedPointer<CRectangleElement> m_root;
  CSharedPointer<CImageElement> m_coverImage;
  CSharedPointer<CTextElement> m_titleText;
  CSharedPointer<CTextElement> m_artistText;
  CSharedPointer<CTextElement> m_timeText;
};

} // namespace UI::Components

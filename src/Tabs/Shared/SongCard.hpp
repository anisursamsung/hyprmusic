#pragma once
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <functional>
#include <string>

namespace UI::Components {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

// ── Config passed at construction ─────────────────────────────────────────
struct SongCardConfig {
  // Style
  CSharedPointer<CPalette> palette;
  std::string              fontFamily;
  int                      rounding     = 5;
  float                    cardHeight   = 70.0f;

  // Initial text & artwork
  std::string title;
  std::string subtitle;  // artist / secondary line
  std::string imagePath = ""; // path to track artwork image

  // Whether the song is currently the active/playing one (colours)
  bool isActive = false;

  // Callbacks
  std::function<void()> onCardBodyClick; // whole card body area clicked
  std::function<void()> onActionClick;   // ⋮ button clicked — caller opens menu
};

// ── Reusable song card component ──────────────────────────────────────────
//
// Usage:
//   auto card = std::make_shared<SongCard>(cfg);
//   layout->addChild(card->build());
//   card->setTitle("New title");
//   card->setSubtitle("New artist");
//   card->setImagePath("/path/to/art.jpg");
//   card->setActive(true);
//
class SongCard {
public:
  explicit SongCard(const SongCardConfig &cfg);

  // Build and return the root element. Call only once per card instance.
  CSharedPointer<CRectangleElement> build();

  // Live update helpers — call any time after build()
  void setTitle(const std::string &title);
  void setSubtitle(const std::string &subtitle);
  void setImagePath(const std::string &path);
  void setActive(bool active);

private:
  SongCardConfig                    m_cfg;

  CSharedPointer<CRectangleElement> m_card;
  CSharedPointer<CRectangleElement> m_artContainer;
  CSharedPointer<CImageElement>     m_artImage;
  CSharedPointer<CTextElement>      m_titleText;
  CSharedPointer<CTextElement>      m_subtitleText;

  bool m_active = false;
};

} // namespace UI::Components

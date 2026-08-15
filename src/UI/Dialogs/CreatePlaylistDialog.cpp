#include "CreatePlaylistDialog.hpp"
#include "../Components/IconProvider.hpp"
#include "../Components/UIFactory.hpp"
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <algorithm>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

void showCreatePlaylistDialog(const CreatePlaylistContext &ctx) {
  if (!ctx.parentWindow)
    return;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;
  auto newPlaylistNameInput = std::make_shared<std::string>("");

  auto windowSize = ctx.parentWindow->pixelSize();
  double popupWidth = 340.0;
  double popupHeight = 170.0;
  double posX = std::max(0.0, (windowSize.x - popupWidth) / 2.0);
  double posY = std::max(0.0, (windowSize.y - popupHeight) / 2.0);

  auto popupWindow =
      CWindowBuilder::begin()
          ->type(HT_WINDOW_POPUP)
          ->parent(ctx.parentWindow)
          ->pos(Hyprutils::Math::Vector2D(posX, posY))
          ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
          ->commence();

  if (!popupWindow)
    return;

  auto root = Components::UIFactory::createCard(palette, 10);
  popupWindow->m_rootElement = root;

  auto cardLayout =
      CColumnLayoutBuilder::begin()
          ->gap(12)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  cardLayout->setMargin(15);

  auto headerText = Components::UIFactory::createHeader(Components::IconProvider::getIcon(Components::IconType::ADD) + " Create New Playlist", palette, fontFamily);
  cardLayout->addChild(headerText);

  auto nameInput = Components::UIFactory::createSearchInput(
      "Playlist name...", "",
      [newPlaylistNameInput](const std::string &text) { *newPlaylistNameInput = text; },
      palette, fontFamily);
  cardLayout->addChild(nameInput);

  auto btnRow =
      CRowLayoutBuilder::begin()
          ->gap(20)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto cancelBtn = Components::UIFactory::createActionButton(
      "Cancel", [popupWindow] { if (popupWindow) popupWindow->close(); }, palette, fontFamily, false);
  btnRow->addChild(cancelBtn);

  auto okBtn = Components::UIFactory::createActionButton(
      "Ok",
      [ctx, popupWindow, newPlaylistNameInput] {
        std::string plName = *newPlaylistNameInput;
        if (!plName.empty()) {
          ctx.runMpdCommand([plName](struct mpd_connection *conn) {
            mpd_run_save(conn, plName.c_str());
            mpd_run_playlist_clear(conn, plName.c_str());
          });
          if (ctx.onCreated) {
            ctx.onCreated(plName);
          }
        }
        if (popupWindow)
          popupWindow->close();
      },
      palette, fontFamily, true);
  btnRow->addChild(okBtn);

  cardLayout->addChild(btnRow);
  root->addChild(cardLayout);

  popupWindow->open();
}

} // namespace UI::Dialogs

#include "PlaylistSelectionDialog.hpp"
#include "BaseDialog.hpp"
#include "Utils/UIFactory.hpp"
#include "Utils/IconProvider.hpp"
#include "Utils/StreamUtils.hpp"
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <algorithm>
#include <thread>
#include <unordered_set>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

CSharedPointer<IWindow> showPlaylistSelectionDialog(const PlaylistSelectionContext &ctx) {
  if (!ctx.parentWindow || ctx.songUri.empty())
    return nullptr;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;

  std::vector<std::string> playlists;
  ctx.runMpdCommand([&playlists](struct mpd_connection *conn) {
    if (conn && mpd_send_list_playlists(conn)) {
      struct mpd_playlist *pl;
      while ((pl = mpd_recv_playlist(conn)) != NULL) {
        const char *name = mpd_playlist_get_path(pl);
        if (name) {
          playlists.push_back(std::string(name));
        }
        mpd_playlist_free(pl);
      }
      mpd_response_finish(conn);
    }
  });

  if (playlists.empty()) {
    if (ctx.showNotification)
      ctx.showNotification("No playlists available");
    return nullptr;
  }

  double popupWidth = 320.0;
  double popupHeight = std::min(380.0, 60.0 + playlists.size() * 42.0 + 50.0);

  auto res = BaseDialog::createCenteredModal(
      ctx.parentWindow,
      Hyprutils::Math::Vector2D(popupWidth, popupHeight),
      palette, 10.0f, 10, 15);

  if (!res.window)
    return nullptr;

  if (ctx.onWindowCreated) {
    ctx.onWindowCreated(res.window);
  }

  auto popupWindow = res.window;

  auto headerText = BaseDialog::createHeader(
      (ctx.moveFromSongPos >= 0) ? "Move to Playlist" : "Add to Playlist",
      Components::IconProvider::getIcon(Components::IconType::FOLDER),
      palette, fontFamily);
  res.contentLayout->addChild(headerText);

  auto scrollArea =
      CScrollAreaBuilder::begin()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.78F}))
          ->scrollY(true)
          ->commence();

  auto listLayout =
      CColumnLayoutBuilder::begin()
          ->gap(6)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  listLayout->setMargin(4);

  for (const auto &plName : playlists) {
    auto plItem = CRectangleBuilder::begin()
                      ->color([palette] {
                        return palette ? palette->m_colors.base
                                       : CHyprColor(0.15, 0.15, 0.15, 1.0);
                      })
                      ->rounding(palette ? palette->m_vars.smallRounding : 5)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                          CDynamicSize::HT_SIZE_ABSOLUTE,
                                          {1.0F, 36.0F}))
                      ->commence();

    auto plRow = CRowLayoutBuilder::begin()
                     ->gap(8)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_PERCENT,
                                         {1.0F, 1.0F}))
                     ->commence();
    plRow->setMargin(6);

    auto iconText = CTextBuilder::begin()
                        ->text(Components::IconProvider::getIcon(Components::IconType::FOLDER))
                        ->color([palette] {
                          return palette ? palette->m_colors.text
                                         : CHyprColor(1.0, 1.0, 1.0, 1.0);
                        })
                        ->fontFamily(std::string(fontFamily))
                        ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {20.0F, 20.0F}))
                        ->commence();
    iconText->setGrow(false);
    plRow->addChild(iconText);

    auto itemText =
        CTextBuilder::begin()
            ->text(std::string(plName))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->align(HT_FONT_ALIGN_LEFT)
            ->commence();
    itemText->setGrow(true);
    plRow->addChild(itemText);
    plItem->addChild(plRow);

    plItem->setReceivesMouse(true);
    plItem->setMouseButton([ctx, popupWindow, plName](Input::eMouseButton button, bool down) {
      if (button == Input::MOUSE_BUTTON_LEFT && !down) {
        std::string songUri = ctx.songUri;
        int moveFromSongPos = ctx.moveFromSongPos;
        std::string currentPl = ctx.currentSelectedPlaylist;

        auto safeNotify = [ctx](const std::string &msg) {
          if (ctx.showNotification && ctx.backend) {
            ctx.backend->addTimer(
                std::chrono::milliseconds(1),
                [ctx, msg](CAtomicSharedPointer<CTimer>, void *) {
                  if (ctx.showNotification)
                    ctx.showNotification(msg);
                },
                nullptr);
          }
        };

        std::thread([ctx, plName, songUri, moveFromSongPos, currentPl, safeNotify]() {
          std::string finalUri = songUri;
          if (songUri.rfind("http://", 0) == 0 ||
              songUri.rfind("https://", 0) == 0) {
            if (songUri.find("youtube.com") != std::string::npos ||
                songUri.find("youtu.be") != std::string::npos) {
              safeNotify(Components::IconProvider::getIcon(Components::IconType::LOADING) + " Resolving YouTube stream...");

              std::string resTitle = "Stream Track";
              std::string resUploader = "";
              std::string realUrl = extractDirectStreamUrl(songUri);
              if (!realUrl.empty()) {
                finalUri = realUrl;
                if (ctx.ytDlpService) {
                  ctx.ytDlpService->setUrlTitle(realUrl, resTitle, resUploader);
                }
              } else {
                safeNotify(Components::IconProvider::getIcon(Components::IconType::CROSS) + " Failed to resolve stream");
                return;
              }
            }
          }

          ctx.runMpdCommand([ctx, plName, finalUri, moveFromSongPos, currentPl,
                             safeNotify](struct mpd_connection *conn) {
            bool alreadyInPlaylist = false;
            if (conn && mpd_send_list_playlist_meta(conn, plName.c_str())) {
              struct mpd_song *s;
              while ((s = mpd_recv_song(conn)) != NULL) {
                const char *pUri = mpd_song_get_uri(s);
                if (pUri && std::string(pUri) == finalUri) {
                  alreadyInPlaylist = true;
                }
                mpd_song_free(s);
              }
              mpd_response_finish(conn);
            }

            if (alreadyInPlaylist) {
              safeNotify("Already added to " + plName);
            } else {
              mpd_run_playlist_add(conn, plName.c_str(), finalUri.c_str());
              if (moveFromSongPos >= 0 && !currentPl.empty()) {
                mpd_run_playlist_delete(conn, currentPl.c_str(),
                                        moveFromSongPos);
              }
              safeNotify("Added to " + plName);
              if (ctx.onPlaylistUpdated)
                ctx.onPlaylistUpdated();
            }
          });
        }).detach();

        if (ctx.backend && popupWindow) {
          ctx.backend->addTimer(
              std::chrono::milliseconds(1),
              [popupWindow](CAtomicSharedPointer<CTimer>, void *) {
                if (popupWindow)
                  popupWindow->close();
              },
              nullptr);
        }
      }
    });

    listLayout->addChild(plItem);
  }

  scrollArea->addChild(listLayout);
  res.contentLayout->addChild(scrollArea);

  auto cancelRow =
      CRowLayoutBuilder::begin()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
          ->commence();

  auto cancelBtn = Components::UIFactory::createActionButton(
      "Close",
      [popupWindow] {
        if (popupWindow)
          popupWindow->close();
      },
      palette, fontFamily, false);
  cancelRow->addChild(cancelBtn);
  res.contentLayout->addChild(cancelRow);

  popupWindow->open();
  return popupWindow;
}

} // namespace UI::Dialogs

#include "AddItemDialog.hpp"
#include "BaseDialog.hpp"
#include "MPD/MPDClient.hpp"
#include "Utils/ClipboardUtils.hpp"
#include "Utils/IconProvider.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include <algorithm>
#include <vector>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace UI::Dialogs {

namespace {

struct DatabaseSong {
  std::string title;
  std::string artist;
  std::string album;
  std::string uri;
};

// --- MPD Data Fetch Helpers ---

std::vector<DatabaseSong> fetchDatabaseSongs(const AddItemDialogContext &ctx) {
  std::vector<DatabaseSong> songs;
  if (!ctx.runMpdCommand)
    return songs;

  ctx.runMpdCommand([&songs](struct mpd_connection *conn) {
    if (!conn || !mpd_send_list_all_meta(conn, ""))
      return;
    struct mpd_entity *entity;
    while ((entity = mpd_recv_entity(conn)) != NULL) {
      if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
        const struct mpd_song *song = mpd_entity_get_song(entity);
        DatabaseSong ds;
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
        const char *uri = mpd_song_get_uri(song);

        ds.title = title ? title : "";
        ds.artist = artist ? artist : "";
        ds.album = album ? album : "";
        ds.uri = uri ? uri : "";

        if (ds.title.empty() && !ds.uri.empty()) {
          size_t slash = ds.uri.find_last_of('/');
          ds.title = (slash != std::string::npos) ? ds.uri.substr(slash + 1)
                                                  : ds.uri;
        }
        songs.push_back(ds);
      }
      mpd_entity_free(entity);
    }
    mpd_response_finish(conn);
  });
  return songs;
}

std::vector<DatabaseSong> fetchQueueSongs(const AddItemDialogContext &ctx) {
  std::vector<DatabaseSong> songs;
  if (!ctx.runMpdCommand)
    return songs;

  ctx.runMpdCommand([&songs](struct mpd_connection *conn) {
    if (!conn || !mpd_send_list_queue_meta(conn))
      return;
    struct mpd_song *s;
    while ((s = mpd_recv_song(conn)) != NULL) {
      DatabaseSong ds;
      const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
      const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
      const char *uri = mpd_song_get_uri(s);
      ds.title = title ? title : "";
      ds.artist = artist ? artist : "";
      ds.uri = uri ? uri : "";
      if (ds.title.empty() && !ds.uri.empty()) {
        size_t slash = ds.uri.find_last_of('/');
        ds.title = (slash != std::string::npos) ? ds.uri.substr(slash + 1)
                                                : ds.uri;
      }
      songs.push_back(ds);
      mpd_song_free(s);
    }
    mpd_response_finish(conn);
  });
  return songs;
}

std::vector<std::string> fetchPlaylists(const AddItemDialogContext &ctx) {
  std::vector<std::string> playlists;
  if (!ctx.runMpdCommand)
    return playlists;

  ctx.runMpdCommand([&playlists](struct mpd_connection *conn) {
    if (!conn || !mpd_send_list_playlists(conn))
      return;
    struct mpd_playlist *pl;
    while ((pl = mpd_recv_playlist(conn)) != NULL) {
      const char *name = mpd_playlist_get_path(pl);
      if (name) {
        playlists.push_back(name);
      }
      mpd_playlist_free(pl);
    }
    mpd_response_finish(conn);
  });
  return playlists;
}

std::vector<DatabaseSong> fetchPlaylistTracks(const AddItemDialogContext &ctx,
                                               const std::string &plName) {
  std::vector<DatabaseSong> tracks;
  if (!ctx.runMpdCommand)
    return tracks;

  ctx.runMpdCommand([plName, &tracks](struct mpd_connection *conn) {
    if (!conn || !mpd_send_list_playlist_meta(conn, plName.c_str()))
      return;
    struct mpd_song *s;
    while ((s = mpd_recv_song(conn)) != NULL) {
      DatabaseSong ds;
      const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
      const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
      const char *uri = mpd_song_get_uri(s);

      ds.title = title ? title : "";
      ds.artist = artist ? artist : "";
      ds.uri = uri ? uri : "";

      if (ds.title.empty() && !ds.uri.empty()) {
        size_t slash = ds.uri.find_last_of('/');
        ds.title = (slash != std::string::npos) ? ds.uri.substr(slash + 1)
                                                : ds.uri;
      }
      tracks.push_back(ds);
      mpd_song_free(s);
    }
    mpd_response_finish(conn);
  });
  return tracks;
}

// --- Action Handlers ---

void addTrackToTarget(const AddItemDialogContext &ctx, const DatabaseSong &song,
                      CSharedPointer<CTextElement> addBtn) {
  if (song.uri.empty())
    return;

  if (ctx.targetType == AddItemTargetType::QUEUE) {
    if (ctx.addSongToQueue) {
      ctx.addSongToQueue(song.uri);
      if (addBtn)
        addBtn->rebuild()->text("✓")->commence();
    }
  } else if (ctx.targetType == AddItemTargetType::PLAYLIST) {
    if (ctx.runMpdCommand) {
      ctx.runMpdCommand([ctx, song, addBtn](struct mpd_connection *conn) {
        if (!conn)
          return;

        auto existingUris =
            MPDUtils::getPlaylistUris(conn, ctx.targetPlaylistName);
        if (existingUris.count(song.uri) > 0) {
          if (ctx.showNotification) {
            ctx.showNotification("Already in " + ctx.targetPlaylistName);
          }
          return;
        }

        if (mpd_run_playlist_add(conn, ctx.targetPlaylistName.c_str(),
                                 song.uri.c_str())) {
          if (ctx.showNotification) {
            ctx.showNotification("Added song to " + ctx.targetPlaylistName);
          }
          if (addBtn) {
            addBtn->rebuild()->text("✓")->commence();
          }
          if (ctx.refreshCallback) {
            ctx.refreshCallback();
          }
        }
      });
    }
  }
}

void addPlaylistToTarget(const AddItemDialogContext &ctx,
                         const std::string &plName,
                         CSharedPointer<CTextElement> addBtn) {
  if (!ctx.runMpdCommand)
    return;

  ctx.runMpdCommand([ctx, plName, addBtn](struct mpd_connection *conn) {
    if (!conn)
      return;

    if (ctx.targetType == AddItemTargetType::QUEUE) {
      auto queueUris = MPDUtils::getQueueUris(conn);
      auto playlistUris = MPDUtils::getPlaylistUris(conn, plName);

      int addedCount = 0;
      int skippedCount = 0;
      for (const auto &songUri : playlistUris) {
        if (queueUris.find(songUri) != queueUris.end()) {
          skippedCount++;
        } else {
          if (mpd_run_add(conn, songUri.c_str())) {
            addedCount++;
          }
        }
      }

      if (ctx.showNotification) {
        if (addedCount > 0 && skippedCount > 0) {
          ctx.showNotification(
              "Added " + std::to_string(addedCount) + " songs (" +
              std::to_string(skippedCount) + " already in que)");
        } else if (addedCount > 0) {
          ctx.showNotification("Added " + std::to_string(addedCount) +
                               " songs to queue");
        } else {
          ctx.showNotification("All songs are already in que");
        }
      }
      if (addedCount > 0 && addBtn) {
        addBtn->rebuild()->text("✓")->commence();
      }
      if (ctx.refreshCallback) {
        ctx.refreshCallback();
      }
    } else if (ctx.targetType == AddItemTargetType::PLAYLIST) {
      auto existingUris =
          MPDUtils::getPlaylistUris(conn, ctx.targetPlaylistName);

      std::vector<std::string> urisToAdd;
      if (mpd_send_list_playlist_meta(conn, plName.c_str())) {
        struct mpd_song *s;
        while ((s = mpd_recv_song(conn)) != NULL) {
          const char *uri = mpd_song_get_uri(s);
          if (uri && existingUris.find(uri) == existingUris.end()) {
            urisToAdd.push_back(uri);
          }
          mpd_song_free(s);
        }
        mpd_response_finish(conn);
      }

      if (urisToAdd.empty()) {
        if (ctx.showNotification) {
          ctx.showNotification("All songs already in " +
                               ctx.targetPlaylistName);
        }
        return;
      }

      int addedCount = 0;
      for (const auto &uri : urisToAdd) {
        if (mpd_run_playlist_add(conn, ctx.targetPlaylistName.c_str(),
                                 uri.c_str())) {
          addedCount++;
        }
      }

      if (ctx.showNotification) {
        ctx.showNotification("Added " + std::to_string(addedCount) +
                             " songs from '" + plName + "' to " +
                             ctx.targetPlaylistName);
      }
      if (addBtn) {
        addBtn->rebuild()->text("✓")->commence();
      }
      if (ctx.refreshCallback) {
        ctx.refreshCallback();
      }
    }
  });
}

// --- UI Building Helpers ---

CSharedPointer<CRowLayoutElement>
createHeaderRow(CSharedPointer<CPalette> palette, const std::string &fontFamily,
                const std::string &title, std::function<void()> onBackClick = nullptr) {
  auto headerRow =
      CRowLayoutBuilder::begin()
          ->gap(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  if (onBackClick) {
    auto backCircleBg =
        CRectangleBuilder::begin()
            ->color([palette] {
              return palette ? palette->m_colors.alternateBase
                             : CHyprColor(0.2, 0.2, 0.2, 1.0);
            })
            ->rounding(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {24.0F, 24.0F}))
            ->commence();

    backCircleBg->setReceivesMouse(true);
    backCircleBg->setMouseButton(
        [onBackClick](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            onBackClick();
          }
        });

    auto backBtnText =
        CTextBuilder::begin()
            ->text(Components::IconProvider::getIcon(Components::IconType::BACK))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(Components::IconProvider::getCustomFontFamily())
            ->fontSize(CFontSize(CFontSize::HT_FONT_ABSOLUTE, 14.0f))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {14.0F, 20.0F}))
            ->commence();
    backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    backBtnText->setAbsolutePosition({-1.0F, 0.0F});
    backCircleBg->addChild(backBtnText);
    headerRow->addChild(backCircleBg);
  }

  auto titleText =
      CTextBuilder::begin()
          ->text(std::string(title))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0, 1.0, 1.0, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  titleText->setGrow(true);
  headerRow->addChild(titleText);
  return headerRow;
}

std::pair<CSharedPointer<CScrollAreaElement>, CSharedPointer<CColumnLayoutElement>>
createScrollableListContainer() {
  auto scrollArea =
      CScrollAreaBuilder::begin()
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  scrollArea->setGrow(true);

  auto listLayout =
      CColumnLayoutBuilder::begin()
          ->gap(4)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  scrollArea->addChild(listLayout);
  return {scrollArea, listLayout};
}

CSharedPointer<CRectangleElement>
renderSongRow(CSharedPointer<CPalette> palette, const std::string &fontFamily,
              const std::string &displayText, const std::string &icon,
              CSharedPointer<CScrollAreaElement> scrollArea,
              std::function<void(CSharedPointer<CTextElement>)> onAddClick) {
  auto songRow = CRectangleBuilder::begin()
                     ->color([palette] {
                       return palette ? palette->m_colors.alternateBase
                                      : CHyprColor(0.18, 0.18, 0.18, 1.0);
                     })
                     ->rounding(6)
                     ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                         CDynamicSize::HT_SIZE_ABSOLUTE,
                                         {1.0F, 34.0F}))
                     ->commence();

  auto innerRow = CRowLayoutBuilder::begin()
                      ->gap(8)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                          CDynamicSize::HT_SIZE_PERCENT,
                                          {1.0F, 1.0F}))
                      ->commence();
  innerRow->setMargin(4);

  auto songText = CTextBuilder::begin()
                      ->text(icon + " " + displayText)
                      ->color([palette] {
                        return palette ? palette->m_colors.text
                                       : CHyprColor(1.0, 1.0, 1.0, 1.0);
                      })
                      ->fontFamily(std::string(fontFamily))
                      ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                          CDynamicSize::HT_SIZE_PERCENT,
                                          {1.0F, 1.0F}))
                      ->align(HT_FONT_ALIGN_LEFT)
                      ->noEllipsize(false)
                      ->commence();

  auto textContainer =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT,
                              {0.75F, 1.0F}))
          ->commence();
  textContainer->setGrow(true);
  textContainer->addChild(songText);
  innerRow->addChild(textContainer);

  auto addBtn = CTextBuilder::begin()
                    ->text(Components::IconProvider::getIcon(
                        Components::IconType::ADD))
                    ->color([palette] {
                      return palette ? palette->m_colors.text
                                     : CHyprColor(1.0, 1.0, 1.0, 1.0);
                    })
                    ->fontFamily(std::string(fontFamily))
                    ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                    ->interactable(true)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                        CDynamicSize::HT_SIZE_ABSOLUTE,
                                        {20.0F, 24.0F}))
                    ->commence();

  songRow->setReceivesMouse(true);
  songRow->setMouseAxis([scrollArea](Input::eAxisAxis axis, float delta) {
    if (axis == Input::AXIS_AXIS_VERTICAL && scrollArea) {
      auto cur = scrollArea->getCurrentScroll();
      scrollArea->setScroll({cur.x, std::max(0.0, cur.y + delta * 25.0)});
    }
  });

  addBtn->setReceivesMouse(true);
  addBtn->setMouseButton(
      [onAddClick, addBtn](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (onAddClick)
            onAddClick(addBtn);
        }
      });

  innerRow->addChild(addBtn);
  songRow->addChild(innerRow);
  return songRow;
}

// --- Dialog Subviews ---

void showMainOptionsView(CSharedPointer<CRectangleElement> root,
                         CSharedPointer<CColumnLayoutElement> cardLayout,
                         CSharedPointer<CPalette> palette,
                         const std::string &fontFamily,
                         CSharedPointer<IWindow> popupWindow,
                         const AddItemDialogContext &ctx,
                         std::function<void()> showAddStream,
                         std::function<void()> showDatabaseSelector,
                         std::function<void()> showQueueSelector,
                         std::function<void()> showPlaylistSelector) {
  cardLayout->clearChildren();

  std::string headerTitle =
      (ctx.targetType == AddItemTargetType::QUEUE)
          ? Components::IconProvider::getIcon(Components::IconType::ADD) +
                " Add Item to Queue"
          : Components::IconProvider::getIcon(Components::IconType::ADD) +
                " Add Item to Playlist";

  auto headerRow = createHeaderRow(palette, fontFamily, headerTitle);
  cardLayout->addChild(headerRow);

  auto addOption = [&](const std::string &iconTitle,
                       std::function<void()> onClick) {
    auto optionCard = CRectangleBuilder::begin()
                          ->color([palette] {
                            return palette
                                       ? palette->m_colors.alternateBase
                                       : CHyprColor(0.18, 0.18, 0.18, 1.0);
                          })
                          ->rounding(8)
                          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                              CDynamicSize::HT_SIZE_ABSOLUTE,
                                              {1.0F, 44.0F}))
                          ->commence();

    auto optionRow =
        CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    optionRow->setMargin(10);

    auto titleText =
        CTextBuilder::begin()
            ->text(std::string(iconTitle))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    titleText->setGrow(true);
    optionRow->addChild(titleText);
    optionCard->addChild(optionRow);

    optionCard->setReceivesMouse(true);
    optionCard->setMouseButton(
        [onClick](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (onClick)
              onClick();
          }
        });

    cardLayout->addChild(optionCard);
  };

  addOption(Components::IconProvider::getIcon(Components::IconType::STREAM) +
                " Add Stream",
            showAddStream);

  addOption(Components::IconProvider::getIcon(Components::IconType::NAV_DATABASE) +
                " Add from Database",
            showDatabaseSelector);

  if (ctx.targetType == AddItemTargetType::PLAYLIST) {
    addOption(Components::IconProvider::getIcon(Components::IconType::NAV_QUEUE) +
                  " Add from Queue",
              showQueueSelector);
  }

  std::string plTitle =
      (ctx.targetType == AddItemTargetType::QUEUE)
          ? Components::IconProvider::getIcon(Components::IconType::NAV_PLAYLIST) +
                " Add from Playlist"
          : Components::IconProvider::getIcon(Components::IconType::NAV_PLAYLIST) +
                " Add from Another Playlist";
  addOption(plTitle, showPlaylistSelector);

  auto cancelBtn =
      CTextBuilder::begin()
          ->text("Cancel")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.7, 0.7, 0.7, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->interactable(true)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->align(HT_FONT_ALIGN_CENTER)
          ->commence();
  cancelBtn->setReceivesMouse(true);
  cancelBtn->setMouseButton(
      [popupWindow](Input::eMouseButton button, bool down) {
        if (button == Input::MOUSE_BUTTON_LEFT && !down) {
          if (popupWindow)
            popupWindow->close();
        }
      });
  cardLayout->addChild(cancelBtn);

  root->addChild(cardLayout);
  root->forceReposition();
}

void showAddStreamView(CSharedPointer<CRectangleElement> root,
                       CSharedPointer<CColumnLayoutElement> cardLayout,
                       CSharedPointer<CPalette> palette,
                       const std::string &fontFamily,
                       const AddItemDialogContext &ctx,
                       std::function<void()> onBack) {
  cardLayout->clearChildren();

  auto headerRow = createHeaderRow(palette, fontFamily, "Add Stream Link", onBack);
  cardLayout->addChild(headerRow);

  auto inputRow =
      CRowLayoutBuilder::begin()
          ->gap(8)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 36.0F}))
          ->commence();

  auto urlInputPtr = std::make_shared<std::string>("");
  auto urlInput =
      CTextboxBuilder::begin()
          ->placeholder("Paste stream or audio URL (http://...)")
          ->onTextEdited([urlInputPtr](CSharedPointer<CTextboxElement>,
                                      const std::string &text) { *urlInputPtr = text; })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 36.0F}))
          ->commence();
  urlInput->setGrow(true);
  inputRow->addChild(urlInput);

  auto pasteBtn =
      CButtonBuilder::begin()
          ->label(Components::IconProvider::getIcon(Components::IconType::PASTE))
          ->alignText(HT_FONT_ALIGN_CENTER)
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->onMainClick([urlInput, urlInputPtr](CSharedPointer<CButtonElement>) {
            std::string pasted = Utils::readFromClipboard();
            if (!pasted.empty()) {
              urlInput->rebuild()
                  ->defaultText(std::string(pasted))
                  ->commence();
              *urlInputPtr = pasted;
            }
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {36.0F, 36.0F}))
          ->commence();
  pasteBtn->setGrow(false);
  inputRow->addChild(pasteBtn);

  auto addBtn =
      CTextBuilder::begin()
          ->text(Components::IconProvider::getIcon(Components::IconType::ADD))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0, 1.0, 1.0, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
          ->interactable(true)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {32.0F, 36.0F}))
          ->align(HT_FONT_ALIGN_CENTER)
          ->commence();

  auto doAddStream = [ctx, urlInputPtr, addBtn]() {
    std::string url = *urlInputPtr;
    if (url.empty())
      return;

    if (ctx.targetType == AddItemTargetType::QUEUE) {
      if (ctx.addSongToQueue) {
        ctx.addSongToQueue(url);
        if (addBtn)
          addBtn->rebuild()->text("✓")->commence();
      }
    } else if (ctx.targetType == AddItemTargetType::PLAYLIST) {
      if (ctx.runMpdCommand) {
        ctx.runMpdCommand([ctx, url, addBtn](struct mpd_connection *conn) {
          if (!conn)
            return;

          auto existingUris =
              MPDUtils::getPlaylistUris(conn, ctx.targetPlaylistName);
          if (existingUris.count(url) > 0) {
            if (ctx.showNotification) {
              ctx.showNotification("Already in " + ctx.targetPlaylistName);
            }
            return;
          }

          if (mpd_run_playlist_add(conn, ctx.targetPlaylistName.c_str(),
                                   url.c_str())) {
            if (ctx.showNotification) {
              ctx.showNotification("Added stream to " + ctx.targetPlaylistName);
            }
            if (addBtn) {
              addBtn->rebuild()->text("✓")->commence();
            }
            if (ctx.refreshCallback) {
              ctx.refreshCallback();
            }
          }
        });
      }
    }
  };

  addBtn->setReceivesMouse(true);
  addBtn->setMouseButton([doAddStream](Input::eMouseButton button, bool down) {
    if (button == Input::MOUSE_BUTTON_LEFT && !down) {
      doAddStream();
    }
  });

  inputRow->addChild(addBtn);
  cardLayout->addChild(inputRow);

  root->addChild(cardLayout);
  root->forceReposition();
}

void showDatabaseSelectorView(CSharedPointer<CRectangleElement> root,
                              CSharedPointer<CColumnLayoutElement> cardLayout,
                              CSharedPointer<CPalette> palette,
                              const std::string &fontFamily,
                              const AddItemDialogContext &ctx,
                              std::function<void()> onBack) {
  cardLayout->clearChildren();

  std::vector<DatabaseSong> songs = fetchDatabaseSongs(ctx);

  auto headerRow = createHeaderRow(palette, fontFamily, "Select from Database", onBack);
  cardLayout->addChild(headerRow);

  auto [scrollArea, listLayout] = createScrollableListContainer();

  if (songs.empty()) {
    auto emptyText =
        CTextBuilder::begin()
            ->text("No songs found in database")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.6, 0.6, 0.6, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    listLayout->addChild(emptyText);
  } else {
    for (const auto &song : songs) {
      std::string labelStr = song.title.empty() ? song.artist : song.title;
      std::string iconStr =
          Components::IconProvider::getIcon(Components::IconType::MUSIC_NOTE);

      auto songRow = renderSongRow(
          palette, fontFamily, labelStr, iconStr, scrollArea,
          [ctx, song](CSharedPointer<CTextElement> addBtn) {
            addTrackToTarget(ctx, song, addBtn);
          });
      listLayout->addChild(songRow);
    }
  }

  cardLayout->addChild(scrollArea);
  root->addChild(cardLayout);
  root->forceReposition();
}

void showQueueSelectorView(CSharedPointer<CRectangleElement> root,
                           CSharedPointer<CColumnLayoutElement> cardLayout,
                           CSharedPointer<CPalette> palette,
                           const std::string &fontFamily,
                           const AddItemDialogContext &ctx,
                           std::function<void()> onBack) {
  cardLayout->clearChildren();

  std::vector<DatabaseSong> songs = fetchQueueSongs(ctx);

  auto headerRow = createHeaderRow(palette, fontFamily, "Select from Queue", onBack);
  cardLayout->addChild(headerRow);

  auto [scrollArea, listLayout] = createScrollableListContainer();

  if (songs.empty()) {
    auto emptyText =
        CTextBuilder::begin()
            ->text("Queue is empty")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.6, 0.6, 0.6, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    listLayout->addChild(emptyText);
  } else {
    for (const auto &song : songs) {
      std::string labelStr = song.title.empty() ? song.artist : song.title;
      std::string iconStr =
          Components::IconProvider::getIcon(Components::IconType::MUSIC_NOTE);

      auto songRow = renderSongRow(
          palette, fontFamily, labelStr, iconStr, scrollArea,
          [ctx, song](CSharedPointer<CTextElement> addBtn) {
            addTrackToTarget(ctx, song, addBtn);
          });
      listLayout->addChild(songRow);
    }
  }

  cardLayout->addChild(scrollArea);
  root->addChild(cardLayout);
  root->forceReposition();
}

void showPlaylistSelectorView(
    CSharedPointer<CRectangleElement> root,
    CSharedPointer<CColumnLayoutElement> cardLayout,
    CSharedPointer<CPalette> palette, const std::string &fontFamily,
    const AddItemDialogContext &ctx, std::function<void()> onBack,
    std::function<void(const std::string &)> showPlaylistTracks) {
  cardLayout->clearChildren();

  std::vector<std::string> playlists = fetchPlaylists(ctx);

  auto headerRow = createHeaderRow(palette, fontFamily, "Select Playlist", onBack);
  cardLayout->addChild(headerRow);

  auto [scrollArea, listLayout] = createScrollableListContainer();

  if (playlists.empty()) {
    auto emptyText =
        CTextBuilder::begin()
            ->text("No playlists found")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.6, 0.6, 0.6, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    listLayout->addChild(emptyText);
  } else {
    for (const auto &plName : playlists) {
      if (ctx.targetType == AddItemTargetType::PLAYLIST &&
          plName == ctx.targetPlaylistName) {
        continue;
      }

      std::string playlistIcon =
          Components::IconProvider::getIcon(Components::IconType::NAV_PLAYLIST);

      auto plRow = renderSongRow(
          palette, fontFamily, plName, playlistIcon, scrollArea,
          [ctx, plName](CSharedPointer<CTextElement> addBtn) {
            addPlaylistToTarget(ctx, plName, addBtn);
          });

      // Override click on playlist row background to open playlist tracks
      plRow->setMouseButton(
          [showPlaylistTracks, plName](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down) {
              if (showPlaylistTracks)
                showPlaylistTracks(plName);
            }
          });

      listLayout->addChild(plRow);
    }
  }

  cardLayout->addChild(scrollArea);
  root->addChild(cardLayout);
  root->forceReposition();
}

void showPlaylistTracksView(CSharedPointer<CRectangleElement> root,
                            CSharedPointer<CColumnLayoutElement> cardLayout,
                            CSharedPointer<CPalette> palette,
                            const std::string &fontFamily,
                            const AddItemDialogContext &ctx,
                            const std::string &plName,
                            std::function<void()> onBack) {
  cardLayout->clearChildren();

  std::vector<DatabaseSong> tracks = fetchPlaylistTracks(ctx, plName);

  auto headerRow = createHeaderRow(palette, fontFamily, plName, onBack);
  cardLayout->addChild(headerRow);

  auto [scrollArea, listLayout] = createScrollableListContainer();

  if (tracks.empty()) {
    auto emptyText =
        CTextBuilder::begin()
            ->text("Playlist is empty")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(0.6, 0.6, 0.6, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->align(HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    listLayout->addChild(emptyText);
  } else {
    for (const auto &song : tracks) {
      std::string labelStr = song.title.empty() ? song.artist : song.title;
      std::string iconStr =
          Components::IconProvider::getIcon(Components::IconType::MUSIC_NOTE);

      auto songRow = renderSongRow(
          palette, fontFamily, labelStr, iconStr, scrollArea,
          [ctx, song](CSharedPointer<CTextElement> addBtn) {
            addTrackToTarget(ctx, song, addBtn);
          });
      listLayout->addChild(songRow);
    }
  }

  cardLayout->addChild(scrollArea);
  root->addChild(cardLayout);
  root->forceReposition();
}

} // namespace

CSharedPointer<IWindow> showAddItemDialog(const AddItemDialogContext &ctx) {
  if (!ctx.window)
    return nullptr;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;

  auto res = BaseDialog::createCenteredModal(
      ctx.window,
      Hyprutils::Math::Vector2D(420.0, 440.0),
      palette, 10.0f, 10, 15);

  if (!res.window)
    return nullptr;

  if (ctx.onWindowCreated) {
    ctx.onWindowCreated(res.window);
  }

  auto popupWindow = res.window;
  auto root = res.card;
  auto cardLayout = res.contentLayout;

  auto showMainOptions = std::make_shared<std::function<void()>>();
  auto showAddStream = std::make_shared<std::function<void()>>();
  auto showDatabaseSelector = std::make_shared<std::function<void()>>();
  auto showQueueSelector = std::make_shared<std::function<void()>>();
  auto showPlaylistSelector = std::make_shared<std::function<void()>>();
  auto showPlaylistTracks =
      std::make_shared<std::function<void(const std::string &)>>();

  *showMainOptions = [root, cardLayout, palette, fontFamily, popupWindow, ctx,
                      showAddStream, showDatabaseSelector, showQueueSelector,
                      showPlaylistSelector]() {
    showMainOptionsView(root, cardLayout, palette, fontFamily, popupWindow, ctx,
                        [showAddStream]() { (*showAddStream)(); },
                        [showDatabaseSelector]() { (*showDatabaseSelector)(); },
                        [showQueueSelector]() { (*showQueueSelector)(); },
                        [showPlaylistSelector]() { (*showPlaylistSelector)(); });
  };

  *showAddStream = [root, cardLayout, palette, fontFamily, ctx, showMainOptions]() {
    showAddStreamView(root, cardLayout, palette, fontFamily, ctx,
                      [showMainOptions]() { (*showMainOptions)(); });
  };

  *showDatabaseSelector = [root, cardLayout, palette, fontFamily, ctx,
                           showMainOptions]() {
    showDatabaseSelectorView(root, cardLayout, palette, fontFamily, ctx,
                             [showMainOptions]() { (*showMainOptions)(); });
  };

  *showQueueSelector = [root, cardLayout, palette, fontFamily, ctx,
                        showMainOptions]() {
    showQueueSelectorView(root, cardLayout, palette, fontFamily, ctx,
                          [showMainOptions]() { (*showMainOptions)(); });
  };

  *showPlaylistSelector = [root, cardLayout, palette, fontFamily, ctx,
                           showMainOptions, showPlaylistTracks]() {
    showPlaylistSelectorView(root, cardLayout, palette, fontFamily, ctx,
                             [showMainOptions]() { (*showMainOptions)(); },
                             [showPlaylistTracks](const std::string &plName) {
                               (*showPlaylistTracks)(plName);
                             });
  };

  *showPlaylistTracks = [root, cardLayout, palette, fontFamily, ctx,
                         showPlaylistSelector](const std::string &plName) {
    showPlaylistTracksView(root, cardLayout, palette, fontFamily, ctx, plName,
                           [showPlaylistSelector]() { (*showPlaylistSelector)(); });
  };

  (*showMainOptions)();
  popupWindow->open();
  return popupWindow;
}

} // namespace UI::Dialogs

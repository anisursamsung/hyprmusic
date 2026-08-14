#include "AddItemDialog.hpp"
#include "../../MPDClient.hpp"
#include "../../Utils/ClipboardUtils.hpp"
#include "../../Utils/StreamUtils.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#define private public
#include <hyprtoolkit/element/Text.hpp>
#undef private
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace UI::Dialogs {

struct DatabaseSong {
  std::string title;
  std::string artist;
  std::string album;
  std::string uri;
};

void showAddItemDialog(const AddItemDialogContext &ctx) {
  if (!ctx.window)
    return;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;

  auto windowSize = ctx.window->pixelSize();
  double popupWidth = 420.0;
  double popupHeight = 440.0;
  double posX = std::max(0.0, (windowSize.x - popupWidth) / 2.0);
  double posY = std::max(0.0, (windowSize.y - popupHeight) / 2.0);

  auto popupWindow =
      CWindowBuilder::begin()
          ->type(HT_WINDOW_POPUP)
          ->parent(ctx.window)
          ->pos(Hyprutils::Math::Vector2D(posX, posY))
          ->preferredSize(Hyprutils::Math::Vector2D(popupWidth, popupHeight))
          ->commence();

  if (!popupWindow)
    return;

  auto root =
      CRectangleBuilder::begin()
          ->color([palette] {
            return palette ? palette->m_colors.base
                           : CHyprColor(0.12, 0.12, 0.12, 1.0);
          })
          ->rounding(10)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  popupWindow->m_rootElement = root;

  auto cardLayout =
      CColumnLayoutBuilder::begin()
          ->gap(10)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
          ->commence();
  cardLayout->setMargin(15);

  auto showMainOptions = std::make_shared<std::function<void()>>();
  auto showAddStream = std::make_shared<std::function<void()>>();
  auto showDatabaseSelector = std::make_shared<std::function<void()>>();
  auto showQueueSelector = std::make_shared<std::function<void()>>();
  auto showPlaylistSelector = std::make_shared<std::function<void()>>();
  auto showPlaylistTracks = std::make_shared<std::function<void(const std::string &)>>();

  *showMainOptions = [root, cardLayout, palette, fontFamily, popupWindow, ctx,
                      showAddStream, showDatabaseSelector, showQueueSelector,
                      showPlaylistSelector]() {
    cardLayout->clearChildren();

    auto headerRow =
        CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

    std::string headerTitle = (ctx.targetType == AddItemTargetType::QUEUE)
                                  ? "➕ Add Item to Queue"
                                  : "➕ Add Item to Playlist";

    auto headerText =
        CTextBuilder::begin()
            ->text(std::string(headerTitle))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    headerText->setGrow(true);
    headerRow->addChild(headerText);
    cardLayout->addChild(headerRow);

    auto addOption = [&](const std::string &iconTitle,
                         std::function<void()> onClick) {
      auto optionCard =
          CRectangleBuilder::begin()
              ->color([palette] {
                return palette ? palette->m_colors.alternateBase
                               : CHyprColor(0.18, 0.18, 0.18, 1.0);
              })
              ->rounding(8)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 44.0F}))
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

    addOption("📻 Add Stream", [showAddStream]() { (*showAddStream)(); });

    addOption("🎵 Add from Database",
              [showDatabaseSelector]() { (*showDatabaseSelector)(); });

    if (ctx.targetType == AddItemTargetType::PLAYLIST) {
      addOption("📋 Add from Queue",
                [showQueueSelector]() { (*showQueueSelector)(); });
    }

    std::string plTitle = (ctx.targetType == AddItemTargetType::QUEUE)
                              ? "📁 Add from Playlist"
                              : "📁 Add from Another Playlist";
    addOption(plTitle, [showPlaylistSelector]() { (*showPlaylistSelector)(); });

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
  };

  *showAddStream = [root, cardLayout, palette, fontFamily, ctx, showMainOptions, popupWindow]() {
    cardLayout->clearChildren();

    auto headerRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

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
        [showMainOptions](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            (*showMainOptions)();
          }
        });

    auto backBtnText =
        CTextBuilder::begin()
            ->text("◀")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {14.0F, 20.0F}))
            ->commence();
    backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    backBtnText->setAbsolutePosition({-1.0F, 0.0F});
    backCircleBg->addChild(backBtnText);
    headerRow->addChild(backCircleBg);

    auto titleText =
        CTextBuilder::begin()
            ->text("Add Stream Link")
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
    cardLayout->addChild(headerRow);

    auto inputRow = CRowLayoutBuilder::begin()
                        ->gap(8)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {1.0F, 36.0F}))
                        ->commence();

    auto urlInputPtr = std::make_shared<std::string>("");
    auto urlInput =
        CTextboxBuilder::begin()
            ->placeholder("Paste stream or audio URL (http://...)")
            ->onTextEdited(
                [urlInputPtr](CSharedPointer<CTextboxElement>,
                              const std::string &text) { *urlInputPtr = text; })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 36.0F}))
            ->commence();
    urlInput->setGrow(true);
    inputRow->addChild(urlInput);

    auto pasteBtn = CButtonBuilder::begin()
                        ->label("📋")
                        ->alignText(HT_FONT_ALIGN_CENTER)
                        ->fontFamily(std::string(fontFamily))
                        ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                        ->onMainClick([urlInput, urlInputPtr](CSharedPointer<CButtonElement>) {
                          std::string pasted = Utils::readFromClipboard();
                          if (!pasted.empty()) {
                            urlInput->rebuild()->defaultText(std::string(pasted))->commence();
                            *urlInputPtr = pasted;
                          }
                        })
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {36.0F, 36.0F}))
                        ->commence();
    pasteBtn->setGrow(false);
    inputRow->addChild(pasteBtn);

    auto addBtn = CTextBuilder::begin()
                      ->text("➕")
                      ->color([palette] {
                        return palette ? palette->m_colors.text
                                       : CHyprColor(1.0, 1.0, 1.0, 1.0);
                      })
                      ->fontFamily(std::string(fontFamily))
                      ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
                      ->interactable(true)
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                          CDynamicSize::HT_SIZE_ABSOLUTE,
                                          {32.0F, 36.0F}))
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
                ctx.showNotification("Added stream to " +
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
      }
    };

    addBtn->setReceivesMouse(true);
    addBtn->setMouseButton(
        [doAddStream](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            doAddStream();
          }
        });

    inputRow->addChild(addBtn);
    cardLayout->addChild(inputRow);

    root->addChild(cardLayout);
    root->forceReposition();
  };

  *showDatabaseSelector = [root, cardLayout, palette, fontFamily, ctx,
                           showMainOptions]() {
    cardLayout->clearChildren();

    std::vector<DatabaseSong> songs;
    if (ctx.runMpdCommand) {
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
    }

    auto headerRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

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
        [showMainOptions](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            (*showMainOptions)();
          }
        });

    auto backBtnText =
        CTextBuilder::begin()
            ->text("◀")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {14.0F, 20.0F}))
            ->commence();
    backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    backBtnText->setAbsolutePosition({-1.0F, 0.0F});
    backCircleBg->addChild(backBtnText);
    headerRow->addChild(backCircleBg);

    auto titleText =
        CTextBuilder::begin()
            ->text("Select from Database")
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
    cardLayout->addChild(headerRow);

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
        auto songRow =
            CRectangleBuilder::begin()
                ->color([palette] {
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.18, 0.18, 0.18, 1.0);
                })
                ->rounding(6)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 34.0F}))
                ->commence();

        auto innerRow =
            CRowLayoutBuilder::begin()
                ->gap(8)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                ->commence();
        innerRow->setMargin(4);

        std::string labelStr = song.title.empty() ? song.artist : song.title;
        auto songText =
            CTextBuilder::begin()
                ->text("🎵 " + labelStr)
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1.0, 1.0, 1.0, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
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

        auto addBtn =
            CTextBuilder::begin()
                ->text("➕")
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

        auto doAddTrack = [ctx, song, addBtn]() {
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
                    ctx.showNotification("Already in " +
                                         ctx.targetPlaylistName);
                  }
                  return;
                }

                if (mpd_run_playlist_add(conn, ctx.targetPlaylistName.c_str(),
                                         song.uri.c_str())) {
                  if (ctx.showNotification) {
                    ctx.showNotification("Added song to " +
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
          }
        };

        songRow->setReceivesMouse(true);
        songRow->setMouseAxis([scrollArea](Input::eAxisAxis axis, float delta) {
          if (axis == Input::AXIS_AXIS_VERTICAL && scrollArea) {
            auto cur = scrollArea->getCurrentScroll();
            scrollArea->setScroll({cur.x, std::max(0.0, cur.y + delta * 25.0)});
          }
        });

        addBtn->setReceivesMouse(true);
        addBtn->setMouseButton(
            [doAddTrack](Input::eMouseButton button, bool down) {
              if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                doAddTrack();
              }
            });

        innerRow->addChild(addBtn);
        songRow->addChild(innerRow);
        listLayout->addChild(songRow);
      }
    }

    cardLayout->addChild(scrollArea);

    root->addChild(cardLayout);
    root->forceReposition();
  };

  *showQueueSelector = [root, cardLayout, palette, fontFamily, ctx,
                        showMainOptions]() {
    cardLayout->clearChildren();

    std::vector<DatabaseSong> songs;
    if (ctx.runMpdCommand) {
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
    }

    auto headerRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

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
        [showMainOptions](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            (*showMainOptions)();
          }
        });

    auto backBtnText =
        CTextBuilder::begin()
            ->text("◀")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {14.0F, 20.0F}))
            ->commence();
    backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    backBtnText->setAbsolutePosition({-1.0F, 0.0F});
    backCircleBg->addChild(backBtnText);
    headerRow->addChild(backCircleBg);

    auto titleText =
        CTextBuilder::begin()
            ->text("Select from Queue")
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
    cardLayout->addChild(headerRow);

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
        auto songRow =
            CRectangleBuilder::begin()
                ->color([palette] {
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.18, 0.18, 0.18, 1.0);
                })
                ->rounding(6)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 34.0F}))
                ->commence();

        auto innerRow =
            CRowLayoutBuilder::begin()
                ->gap(8)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                ->commence();
        innerRow->setMargin(4);

        std::string labelStr = song.title.empty() ? song.artist : song.title;
        auto songText =
            CTextBuilder::begin()
                ->text("🎵 " + labelStr)
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1.0, 1.0, 1.0, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
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

        auto addBtn =
            CTextBuilder::begin()
                ->text("➕")
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

        auto doAddTrack = [ctx, song, addBtn]() {
          if (song.uri.empty())
            return;

          if (ctx.targetType == AddItemTargetType::PLAYLIST && ctx.runMpdCommand) {
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
                  ctx.showNotification("Added song to " +
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
        };

        songRow->setReceivesMouse(true);
        songRow->setMouseAxis([scrollArea](Input::eAxisAxis axis, float delta) {
          if (axis == Input::AXIS_AXIS_VERTICAL && scrollArea) {
            auto cur = scrollArea->getCurrentScroll();
            scrollArea->setScroll({cur.x, std::max(0.0, cur.y + delta * 25.0)});
          }
        });

        addBtn->setReceivesMouse(true);
        addBtn->setMouseButton(
            [doAddTrack](Input::eMouseButton button, bool down) {
              if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                doAddTrack();
              }
            });

        innerRow->addChild(addBtn);
        songRow->addChild(innerRow);
        listLayout->addChild(songRow);
      }
    }

    cardLayout->addChild(scrollArea);

    root->addChild(cardLayout);
    root->forceReposition();
  };

  *showPlaylistSelector = [root, cardLayout, palette, fontFamily, ctx,
                           showMainOptions, showPlaylistTracks]() {
    cardLayout->clearChildren();

    std::vector<std::string> playlists;
    if (ctx.runMpdCommand) {
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
    }

    auto headerRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

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
        [showMainOptions](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            (*showMainOptions)();
          }
        });

    auto backBtnText =
        CTextBuilder::begin()
            ->text("◀")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {14.0F, 20.0F}))
            ->commence();
    backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    backBtnText->setAbsolutePosition({-1.0F, 0.0F});
    backCircleBg->addChild(backBtnText);
    headerRow->addChild(backCircleBg);

    auto titleText =
        CTextBuilder::begin()
            ->text("Select Playlist")
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
    cardLayout->addChild(headerRow);

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

        auto plRow =
            CRectangleBuilder::begin()
                ->color([palette] {
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.18, 0.18, 0.18, 1.0);
                })
                ->rounding(6)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 34.0F}))
                ->commence();

        auto innerRow =
            CRowLayoutBuilder::begin()
                ->gap(8)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                ->commence();
        innerRow->setMargin(4);

        auto plText =
            CTextBuilder::begin()
                ->text("📁 " + plName)
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1.0, 1.0, 1.0, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
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
        textContainer->addChild(plText);
        innerRow->addChild(textContainer);

        auto addBtn =
            CTextBuilder::begin()
                ->text("➕")
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

        auto doAddPlaylist = [ctx, plName, addBtn]() {
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
                  ctx.showNotification("Added " + std::to_string(addedCount) +
                                       " songs (" +
                                       std::to_string(skippedCount) +
                                       " already in que)");
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
        };

        plRow->setReceivesMouse(true);
        plRow->setMouseButton(
            [showPlaylistTracks, plName](Input::eMouseButton button, bool down) {
              if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                (*showPlaylistTracks)(plName);
              }
            });
        plRow->setMouseAxis([scrollArea](Input::eAxisAxis axis, float delta) {
          if (axis == Input::AXIS_AXIS_VERTICAL && scrollArea) {
            auto cur = scrollArea->getCurrentScroll();
            scrollArea->setScroll({cur.x, std::max(0.0, cur.y + delta * 25.0)});
          }
        });

        addBtn->setReceivesMouse(true);
        addBtn->setMouseButton(
            [doAddPlaylist](Input::eMouseButton button, bool down) {
              if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                doAddPlaylist();
              }
            });

        innerRow->addChild(addBtn);
        plRow->addChild(innerRow);
        listLayout->addChild(plRow);
      }
    }

    cardLayout->addChild(scrollArea);

    root->addChild(cardLayout);
    root->forceReposition();
  };

  *showPlaylistTracks = [root, cardLayout, palette, fontFamily, ctx,
                         showPlaylistSelector](const std::string &plName) {
    cardLayout->clearChildren();

    std::vector<DatabaseSong> tracks;
    if (ctx.runMpdCommand) {
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
    }

    auto headerRow =
        CRowLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();

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
        [showPlaylistSelector](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            (*showPlaylistSelector)();
          }
        });

    auto backBtnText =
        CTextBuilder::begin()
            ->text("◀")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {14.0F, 20.0F}))
            ->commence();
    backBtnText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
    backBtnText->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
    backBtnText->setAbsolutePosition({-1.0F, 0.0F});
    backCircleBg->addChild(backBtnText);
    headerRow->addChild(backCircleBg);

    auto titleText =
        CTextBuilder::begin()
            ->text(std::string(plName))
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
    cardLayout->addChild(headerRow);

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
        auto songRow =
            CRectangleBuilder::begin()
                ->color([palette] {
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.18, 0.18, 0.18, 1.0);
                })
                ->rounding(6)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 34.0F}))
                ->commence();

        auto innerRow =
            CRowLayoutBuilder::begin()
                ->gap(8)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
                ->commence();
        innerRow->setMargin(4);

        std::string labelStr = song.title.empty() ? song.artist : song.title;
        auto songText =
            CTextBuilder::begin()
                ->text("🎵 " + labelStr)
                ->color([palette] {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1.0, 1.0, 1.0, 1.0);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
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

        auto addBtn =
            CTextBuilder::begin()
                ->text("➕")
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

        auto doAddTrack = [ctx, song, addBtn]() {
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
                    ctx.showNotification("Already in " +
                                         ctx.targetPlaylistName);
                  }
                  return;
                }

                if (mpd_run_playlist_add(conn, ctx.targetPlaylistName.c_str(),
                                         song.uri.c_str())) {
                  if (ctx.showNotification) {
                    ctx.showNotification("Added song to " +
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
          }
        };

        songRow->setReceivesMouse(true);
        songRow->setMouseAxis([scrollArea](Input::eAxisAxis axis, float delta) {
          if (axis == Input::AXIS_AXIS_VERTICAL && scrollArea) {
            auto cur = scrollArea->getCurrentScroll();
            scrollArea->setScroll({cur.x, std::max(0.0, cur.y + delta * 25.0)});
          }
        });

        addBtn->setReceivesMouse(true);
        addBtn->setMouseButton(
            [doAddTrack](Input::eMouseButton button, bool down) {
              if (button == Input::MOUSE_BUTTON_LEFT && !down) {
                doAddTrack();
              }
            });

        innerRow->addChild(addBtn);
        songRow->addChild(innerRow);
        listLayout->addChild(songRow);
      }
    }

    cardLayout->addChild(scrollArea);

    root->addChild(cardLayout);
    root->forceReposition();
  };

  (*showMainOptions)();
  popupWindow->open();
}

} // namespace UI::Dialogs

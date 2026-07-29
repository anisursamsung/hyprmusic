#include "DatabaseView.hpp"
#include "../Dialogs/ActionMenuDialog.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

DatabaseView::DatabaseView(const DatabaseViewContext &ctx) : m_ctx(ctx) {}

void DatabaseView::rebuildUI(CSharedPointer<CRectangleElement> wrapper,
                            struct mpd_connection *conn) {
  (void)conn;
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  if (!m_dbContentLayout) {
    m_tabContentWrapper->clearChildren();

    auto tabMainLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    m_tabContentWrapper->addChild(tabMainLayout);

    auto topControlsCol =
        CColumnLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    topControlsCol->setMargin(10);

    auto topSearchRow =
        CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();

    auto searchBar =
        CTextboxBuilder::begin()
            ->placeholder("Search...")
            ->defaultText(std::string(m_searchQuery))
            ->onTextEdited([this](CSharedPointer<CTextboxElement>,
                                  const std::string &text) {
              m_searchQuery = text;
              m_ctx.backend->addTimer(
                  std::chrono::milliseconds(1),
                  [this](CAtomicSharedPointer<CTimer>, void *) {
                    m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                      populateDatabaseSongs(conn);
                    });
                  },
                  nullptr);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
            ->commence();
    searchBar->setGrow(true);
    topSearchRow->addChild(searchBar);

    auto dbActionsBtn =
        CButtonBuilder::begin()
            ->label("⋮")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              Dialogs::showActionMenuDialog({
                  .options = {"▶ Play All", "➕ Add All to Queue",
                              "🔄 Update Database", "🔍 Rescan Database"},
                  .onSelect =
                      [this](size_t idx, const std::string &) {
                        if (idx == 0) { // ▶ Play All
                          m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                            if (!conn)
                              return;

                            auto dbUris = collectMatchingSongUris(conn);
                            if (dbUris.empty()) {
                              if (m_ctx.showNotification)
                                m_ctx.showNotification("No songs match current search/filter");
                              return;
                            }

                            mpd_run_clear(conn);
                            for (const auto &uri : dbUris) {
                              mpd_run_add(conn, uri.c_str());
                            }
                            mpd_run_play_pos(conn, 0);

                            populateDatabaseSongs(conn);

                            if (m_ctx.showNotification) {
                              m_ctx.showNotification("▶ Playing " + std::to_string(dbUris.size()) +
                                                     " track(s)");
                            }
                          });
                        } else if (idx == 1) { // ➕ Add All to Queue
                          m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                            if (!conn)
                              return;

                            std::unordered_set<std::string> queueUris;
                            if (mpd_send_list_queue_meta(conn)) {
                              struct mpd_song *s;
                              while ((s = mpd_recv_song(conn)) != NULL) {
                                const char *u = mpd_song_get_uri(s);
                                if (u)
                                  queueUris.insert(std::string(u));
                                mpd_song_free(s);
                              }
                              mpd_response_finish(conn);
                            }

                            auto dbUris = collectMatchingSongUris(conn);
                            int addedCount = 0;
                            for (const auto &uri : dbUris) {
                              if (queueUris.find(uri) == queueUris.end()) {
                                mpd_run_add(conn, uri.c_str());
                                queueUris.insert(uri);
                                addedCount++;
                              }
                            }

                            populateDatabaseSongs(conn);

                            if (m_ctx.showNotification) {
                              if (addedCount > 0)
                                m_ctx.showNotification("➕ Added " + std::to_string(addedCount) +
                                                       " item(s) to Queue");
                              else
                                m_ctx.showNotification("All items are already in Queue");
                            }
                          });
                        } else if (idx == 2) { // 🔄 Update Database
                          m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                            if (conn) {
                              mpd_run_update(conn, nullptr);
                              populateDatabaseSongs(conn);
                            }
                          });
                          if (m_ctx.showNotification)
                            m_ctx.showNotification("🔄 MPD Update Triggered");
                        } else if (idx == 3) { // 🔍 Rescan Database
                          m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                            if (conn) {
                              mpd_run_rescan(conn, nullptr);
                              populateDatabaseSongs(conn);
                            }
                          });
                          if (m_ctx.showNotification)
                            m_ctx.showNotification("🔍 Full Database Rescan Triggered");
                        }
                      },
                  .parentWindow = m_ctx.window,
                  .backend = m_ctx.backend,
                  .palette = m_ctx.palette,
                  .fontFamily = m_ctx.fontFamily});
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {32.0F, 32.0F}))
            ->commence();
    dbActionsBtn->setGrow(false);
    topSearchRow->addChild(dbActionsBtn);

    topControlsCol->addChild(topSearchRow);
    tabMainLayout->addChild(topControlsCol);

    auto scrollArea =
        CScrollAreaBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 0.90F}))
            ->scrollY(true)
            ->commence();

    m_dbContentLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_dbContentLayout->setMargin(5);

    scrollArea->addChild(m_dbContentLayout);
    tabMainLayout->addChild(scrollArea);
  }

  m_dbContentLayout->clearChildren();

  auto loadingText =
      CTextBuilder::begin()
          ->text(std::string("⏳ Loading Database..."))
          ->color([palette] {
            return palette ? palette->m_colors.accent
                           : CHyprColor(0.2, 0.8, 0.4, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
          ->align(HT_FONT_ALIGN_CENTER)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  m_dbContentLayout->addChild(loadingText);

  m_dbContentLayout->forceReposition();
  m_tabContentWrapper->forceReposition();

  m_ctx.backend->addTimer(
      std::chrono::milliseconds(1),
      [this](CAtomicSharedPointer<CTimer>, void *) {
        m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
          populateDatabaseSongs(conn);
        });
      },
      nullptr);
}

void DatabaseView::populateDatabaseSongs(struct mpd_connection *conn) {
  if (!m_dbContentLayout)
    return;

  m_dbContentLayout->clearChildren();

  auto palette = m_ctx.palette;
  int rounding = palette ? palette->m_vars.smallRounding : 5;
  std::string fontFamily = m_ctx.fontFamily;

  std::unordered_set<std::string> queueUris;
  if (conn && mpd_send_list_queue_meta(conn)) {
    struct mpd_song *s;
    while ((s = mpd_recv_song(conn)) != NULL) {
      const char *uri = mpd_song_get_uri(s);
      if (uri) {
        queueUris.insert(std::string(uri));
      }
      mpd_song_free(s);
    }
    mpd_response_finish(conn);
  }

  if (!conn || !mpd_send_list_all_meta(conn, NULL)) {
    std::cerr << "MPD: Failed to send list database command" << std::endl;
    return;
  }

  bool foundAny = false;
  struct mpd_entity *entity;
  int trackNum = 1;
  while ((entity = mpd_recv_entity(conn)) != NULL) {
    if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
      const struct mpd_song *s = mpd_entity_get_song(entity);
      const char *uri = mpd_song_get_uri(s);
      if (uri) {
        std::string songUri(uri);

        const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
        const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
        std::string displayTitle;
        if (title) {
          std::string artistStr = artist ? artist : "Unknown Artist";
          displayTitle = std::string(title) + " - " + artistStr;
        } else {
          displayTitle = songUri;
        }

        if (!m_searchQuery.empty()) {
          std::string query = m_searchQuery;
          std::string titleLower = displayTitle;
          std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
          std::transform(query.begin(), query.end(), query.begin(), ::tolower);
          if (titleLower.find(query) == std::string::npos) {
            mpd_entity_free(entity);
            continue;
          }
        }

        foundAny = true;
        bool inQueue = (queueUris.find(songUri) != queueUris.end());
        std::string indexStr = std::to_string(trackNum++) + ". ";

        auto songItem =
            CRectangleBuilder::begin()
                ->color([palette] {
                  return palette ? palette->m_colors.base
                                 : CHyprColor(0.15, 0.15, 0.15, 1.0);
                })
                ->rounding(rounding)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 40.0F}))
                ->commence();

        auto rowLayout = CRowLayoutBuilder::begin()
                             ->gap(10)
                             ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                                 CDynamicSize::HT_SIZE_PERCENT,
                                                 {1.0F, 1.0F}))
                             ->commence();
        rowLayout->setMargin(6);

        auto songText = CTextBuilder::begin()
                            ->text(std::string(indexStr + displayTitle))
                            ->color([palette, inQueue] {
                              if (inQueue) {
                                return palette
                                           ? palette->m_colors.accent
                                           : CHyprColor(0.2, 0.8, 0.4, 1.0);
                              }
                              return palette ? palette->m_colors.text
                                             : CHyprColor(1, 1, 1, 1);
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
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_PERCENT,
                                    {1.0F, 1.0F}))
                ->commence();
        textContainer->setGrow(true);
        textContainer->addChild(songText);
        rowLayout->addChild(textContainer);

        auto playTrackBtn =
            CTextBuilder::begin()
                ->text(std::string("▶"))
                ->color([palette, inQueue] {
                  if (inQueue) {
                    return palette ? palette->m_colors.accent
                                   : CHyprColor(0.2, 0.8, 0.4, 1.0);
                  }
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1, 1, 1, 1);
                })
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->interactable(true)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 28.0F}))
                ->commence();
        playTrackBtn->setReceivesMouse(true);
        playTrackBtn->setMouseButton([this, songUri](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (!songUri.empty() && m_ctx.playSongFromUri) {
              m_ctx.playSongFromUri(songUri);
            }
          }
        });
        playTrackBtn->setGrow(false);
        rowLayout->addChild(playTrackBtn);

        auto actionBtn =
            CButtonBuilder::begin()
                ->label("⋮")
                ->alignText(HT_FONT_ALIGN_CENTER)
                ->fontFamily(std::string(fontFamily))
                ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                ->onMainClick([this, songUri](CSharedPointer<CButtonElement>) {
                  Dialogs::showActionMenuDialog({
                      .options = {"➕ Add to Queue", "📁 Add to Playlist"},
                      .onSelect =
                          [this, songUri](size_t idx, const std::string &) {
                            if (idx == 0) { // ➕ Add to Queue
                              if (m_ctx.addSongToQueue)
                                m_ctx.addSongToQueue(songUri);
                            } else if (idx == 1) { // 📁 Add to Playlist
                              if (!songUri.empty() && m_ctx.showPlaylistSelectionDialog) {
                                m_ctx.showPlaylistSelectionDialog(songUri);
                              }
                            }
                          },
                      .parentWindow = m_ctx.window,
                      .backend = m_ctx.backend,
                      .palette = m_ctx.palette,
                      .fontFamily = m_ctx.fontFamily});
                })
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {28.0F, 28.0F}))
                ->commence();
        actionBtn->setGrow(false);
        rowLayout->addChild(actionBtn);

        songItem->addChild(rowLayout);
        m_dbContentLayout->addChild(songItem);
      }
    }
    mpd_entity_free(entity);
  }
  mpd_response_finish(conn);

  if (!foundAny) {
    auto emptyText =
        CTextBuilder::begin()
            ->text(std::string("No songs found in Database"))
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
    m_dbContentLayout->addChild(emptyText);
  }

  m_dbContentLayout->forceReposition();
  if (m_tabContentWrapper)
    m_tabContentWrapper->forceReposition();
}

std::vector<std::string> DatabaseView::collectMatchingSongUris(struct mpd_connection *conn) {
  std::vector<std::string> uris;
  if (!conn || !mpd_send_list_all_meta(conn, NULL)) {
    return uris;
  }

  struct mpd_entity *entity;
  while ((entity = mpd_recv_entity(conn)) != NULL) {
    if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
      const struct mpd_song *s = mpd_entity_get_song(entity);
      const char *uri = mpd_song_get_uri(s);
      if (uri) {
        std::string songUri(uri);
        const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
        const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
        std::string displayTitle;
        if (title) {
          std::string artistStr = artist ? artist : "Unknown Artist";
          displayTitle = std::string(title) + " - " + artistStr;
        } else {
          displayTitle = songUri;
        }

        if (!m_searchQuery.empty()) {
          std::string query = m_searchQuery;
          std::string titleLower = displayTitle;
          std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
          std::transform(query.begin(), query.end(), query.begin(), ::tolower);
          if (titleLower.find(query) == std::string::npos) {
            mpd_entity_free(entity);
            continue;
          }
        }
        uris.push_back(songUri);
      }
    }
    mpd_entity_free(entity);
  }
  mpd_response_finish(conn);
  return uris;
}

} // namespace UI::Views

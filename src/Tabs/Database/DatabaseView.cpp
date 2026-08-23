#include "DatabaseView.hpp"
#include "Dialogs/ActionMenuDialog.hpp"
#include "Dialogs/DialogCoordinator.hpp"
#include "Tabs/Shared/SongCard.hpp"
#include "Utils/IconProvider.hpp"
#include "Utils/ArtworkUtils.hpp"
#include <algorithm>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <unordered_set>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace {

struct DbSongItem {
  std::string songUri;
  std::string title;
  std::string artist;
};

// --- Helper: Fetch & Filter Songs from MPD Database ---

std::vector<DbSongItem> fetchMatchingDbSongs(struct mpd_connection *conn,
                                              const std::string &searchQuery) {
  std::vector<DbSongItem> dbSongs;
  if (!conn || !mpd_send_list_all_meta(conn, NULL)) {
    return dbSongs;
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

        std::string displayTitle = title ? std::string(title) : songUri;
        std::string displayArtist = artist ? std::string(artist) : "Unknown Artist";

        if (!searchQuery.empty()) {
          std::string query = searchQuery;
          std::string searchTarget = displayTitle + " " + displayArtist;
          std::transform(searchTarget.begin(), searchTarget.end(),
                         searchTarget.begin(), ::tolower);
          std::transform(query.begin(), query.end(), query.begin(), ::tolower);
          if (searchTarget.find(query) == std::string::npos) {
            mpd_entity_free(entity);
            continue;
          }
        }
        dbSongs.push_back({songUri, displayTitle, displayArtist});
      }
    }
    mpd_entity_free(entity);
  }
  mpd_response_finish(conn);
  return dbSongs;
}

// --- Helper: Render SongCard Component for Database Track ---

std::shared_ptr<UI::Components::SongCard> renderDbSongCard(
    const DatabaseViewContext &ctx, const DbSongItem &item, int trackNum,
    int rounding) {
  std::string songUri = item.songUri;
  std::string indexStr = std::to_string(trackNum) + ". ";

  std::string cachedArt = Utils::getCachedTrackArtwork(songUri);
  std::string artPath =
      cachedArt.empty() ? Utils::getDefaultArtworkPath() : cachedArt;

  return std::make_shared<UI::Components::SongCard>(
      UI::Components::SongCardConfig{
          .palette = ctx.palette,
          .fontFamily = ctx.fontFamily,
          .rounding = rounding,
          .cardHeight = 70.0f,
          .title = indexStr + item.title,
          .subtitle = item.artist,
          .imagePath = artPath,
          .isActive = false,
          .onCardBodyClick =
              [ctx, songUri] {
                if (!songUri.empty() && ctx.playSongFromUri)
                  ctx.playSongFromUri(songUri);
              },
          .onActionClick =
              [ctx, songUri] {
                Dialogs::ActionMenuContext menuCtx{
                    .options = {Components::IconProvider::getIcon(
                                    Components::IconType::PLAY) +
                                    " Play",
                                Components::IconProvider::getIcon(
                                    Components::IconType::ADD_TO_QUEUE) +
                                    " Add to Queue",
                                Components::IconProvider::getIcon(
                                    Components::IconType::ADD_TO_PLAYLIST) +
                                    " Add to Playlist"},
                    .onSelect =
                        [ctx, songUri](size_t idx, const std::string &) {
                          if (idx == 0) {
                            if (!songUri.empty() && ctx.playSongFromUri)
                              ctx.playSongFromUri(songUri);
                          } else if (idx == 1) {
                            if (ctx.addSongToQueue)
                              ctx.addSongToQueue(songUri);
                          } else if (idx == 2) {
                            if (!songUri.empty()) {
                              if (ctx.dialogCoordinator)
                                ctx.dialogCoordinator->showPlaylistSelectionDialog(songUri);
                              else if (ctx.showPlaylistSelectionDialog)
                                ctx.showPlaylistSelectionDialog(songUri);
                            }
                          }
                        },
                    .parentWindow = ctx.window,
                    .backend = ctx.backend,
                    .palette = ctx.palette,
                    .fontFamily = ctx.fontFamily};

                if (ctx.dialogCoordinator)
                  ctx.dialogCoordinator->showActionMenu(menuCtx);
                else
                  Dialogs::showActionMenuDialog(menuCtx);
              }});
}

// --- Helper: Action Menu for Top Bar (Play All, Add All to Queue, Update/Rescan DB) ---

void showDatabaseTopMenu(const DatabaseViewContext &ctx,
                         std::function<std::vector<std::string>(struct mpd_connection *)> collectUris,
                         std::function<void(struct mpd_connection *)> onRefreshNeeded) {
  Dialogs::ActionMenuContext menuCtx{
      .options = {Components::IconProvider::getIcon(Components::IconType::PLAY) +
                      " Play All",
                  Components::IconProvider::getIcon(
                      Components::IconType::ADD_TO_QUEUE) +
                      " Add All to Queue",
                  Components::IconProvider::getIcon(
                      Components::IconType::UPDATE_DB) +
                      " Update Database",
                  Components::IconProvider::getIcon(
                      Components::IconType::RESCAN_DB) +
                      " Rescan Database"},
      .onSelect =
          [ctx, collectUris, onRefreshNeeded](size_t idx, const std::string &) {
            if (idx == 0) { // ▶ Play All
              ctx.runMpdCommand([ctx, collectUris, onRefreshNeeded](struct mpd_connection *conn) {
                if (!conn)
                  return;

                auto dbUris = collectUris(conn);
                if (dbUris.empty()) {
                  if (ctx.showNotification)
                    ctx.showNotification("No songs match current search/filter");
                  return;
                }

                mpd_run_clear(conn);
                for (const auto &uri : dbUris) {
                  mpd_run_add(conn, uri.c_str());
                }
                mpd_run_play_pos(conn, 0);

                if (onRefreshNeeded)
                  onRefreshNeeded(conn);

                if (ctx.showNotification) {
                  ctx.showNotification("▶ Playing " +
                                       std::to_string(dbUris.size()) +
                                       " track(s)");
                }
              });
            } else if (idx == 1) { // ➕ Add All to Queue
              ctx.runMpdCommand([ctx, collectUris, onRefreshNeeded](struct mpd_connection *conn) {
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

                auto dbUris = collectUris(conn);
                int addedCount = 0;
                for (const auto &uri : dbUris) {
                  if (queueUris.find(uri) == queueUris.end()) {
                    mpd_run_add(conn, uri.c_str());
                    queueUris.insert(uri);
                    addedCount++;
                  }
                }

                if (onRefreshNeeded)
                  onRefreshNeeded(conn);

                if (ctx.showNotification) {
                  if (addedCount > 0)
                    ctx.showNotification("➕ Added " +
                                         std::to_string(addedCount) +
                                         " item(s) to Queue");
                  else
                    ctx.showNotification("All items are already in Queue");
                }
              });
            } else if (idx == 2) { // 🔄 Update Database
              ctx.runMpdCommand([ctx, onRefreshNeeded](struct mpd_connection *conn) {
                if (conn) {
                  mpd_run_update(conn, nullptr);
                  if (onRefreshNeeded)
                    onRefreshNeeded(conn);
                }
              });
              if (ctx.showNotification)
                ctx.showNotification("🔄 MPD Update Triggered");
            } else if (idx == 3) { // 🔍 Rescan Database
              ctx.runMpdCommand([ctx, onRefreshNeeded](struct mpd_connection *conn) {
                if (conn) {
                  mpd_run_rescan(conn, nullptr);
                  if (onRefreshNeeded)
                    onRefreshNeeded(conn);
                }
              });
              if (ctx.showNotification)
                ctx.showNotification("🔍 Full Database Rescan Triggered");
            }
          },
      .parentWindow = ctx.window,
      .backend = ctx.backend,
      .palette = ctx.palette,
      .fontFamily = ctx.fontFamily};

  if (ctx.dialogCoordinator)
    ctx.dialogCoordinator->showActionMenu(menuCtx);
  else
    Dialogs::showActionMenuDialog(menuCtx);
}

} // namespace

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
            ->gap(4)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    tabMainLayout->setMargin(20);
    m_tabContentWrapper->addChild(tabMainLayout);

    auto titleHeader =
        CTextBuilder::begin()
            ->text("Database")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H1))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    tabMainLayout->addChild(titleHeader);

    auto topControlsCol =
        CColumnLayoutBuilder::begin()
            ->gap(4)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    topControlsCol->setMargin(5);

    auto topSearchRow =
        CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
            ->commence();

    auto leftSpacer =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_PERCENT, {20.0F, 1.0F}))
            ->commence();
    leftSpacer->setGrow(false);
    topSearchRow->addChild(leftSpacer);

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
            ->multiline(false)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
            ->commence();
    searchBar->setGrow(true);
    topSearchRow->addChild(searchBar);

    auto dbActionsBtn =
        CButtonBuilder::begin()
            ->label(Components::IconProvider::getIcon(Components::IconType::MENU))
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(Components::IconProvider::getCustomFontFamily())
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              showDatabaseTopMenu(
                  m_ctx,
                  [this](struct mpd_connection *c) {
                    return collectMatchingSongUris(c);
                  },
                  [this](struct mpd_connection *c) {
                    populateDatabaseSongs(c);
                  });
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {40.0F, 40.0F}))
            ->commence();
    dbActionsBtn->setGrow(false);
    topSearchRow->addChild(dbActionsBtn);

    auto rightSpacer =
        CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_PERCENT, {20.0F, 1.0F}))
            ->commence();
    rightSpacer->setGrow(false);
    topSearchRow->addChild(rightSpacer);

    topControlsCol->addChild(topSearchRow);
    tabMainLayout->addChild(topControlsCol);

    auto scrollArea =
        CScrollAreaBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
            ->scrollY(true)
            ->commence();
    scrollArea->setGrow(true);

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
          ->text(Components::IconProvider::getIcon(Components::IconType::LOADING) +
                 " Loading Database...")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0, 1.0, 1.0, 1.0);
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

  std::vector<DbSongItem> dbSongs = fetchMatchingDbSongs(conn, m_searchQuery);
  std::vector<DbSongItem> uncachedSongs;

  m_dbSongCards.clear();
  int trackNum = 1;
  bool foundAny = !dbSongs.empty();

  for (const auto &item : dbSongs) {
    std::string songUri = item.songUri;
    std::string cachedArt = Utils::getCachedTrackArtwork(songUri);
    if (cachedArt.empty() && !songUri.empty()) {
      uncachedSongs.push_back(item);
    }

    auto card = renderDbSongCard(m_ctx, item, trackNum++, rounding);
    m_dbSongCards[songUri] = card;
    m_dbContentLayout->addChild(card->build());
  }

  // Non-blocking progressive micro-batched artwork resolution (2 tracks per 15ms batch for uncached items only)
  if (m_ctx.backend && m_ctx.runMpdCommand && !uncachedSongs.empty()) {
    auto stepState = std::make_shared<size_t>(0);
    auto processNextChunk = [this, uncachedSongs, stepState](auto self) -> void {
      if (*stepState >= uncachedSongs.size())
        return;
      m_ctx.runMpdCommand([this, uncachedSongs, stepState, self](struct mpd_connection *conn) {
        size_t limit = std::min(*stepState + 2, uncachedSongs.size());
        for (size_t i = *stepState; i < limit; ++i) {
          const auto &item = uncachedSongs[i];
          std::string resolved = Utils::resolveTrackArtwork(conn, item.songUri);
          if (!resolved.empty()) {
            auto it = m_dbSongCards.find(item.songUri);
            if (it != m_dbSongCards.end() && it->second) {
              it->second->setImagePath(resolved);
            }
          }
        }
        *stepState = limit;
        if (*stepState < uncachedSongs.size()) {
          m_ctx.backend->addTimer(
              std::chrono::milliseconds(15),
              [self](CAtomicSharedPointer<CTimer>, void *) { self(self); },
              nullptr);
        }
      });
    };

    m_ctx.backend->addTimer(
        std::chrono::milliseconds(5),
        [processNextChunk](CAtomicSharedPointer<CTimer>, void *) { processNextChunk(processNextChunk); },
        nullptr);
  }

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
  std::vector<DbSongItem> songs = fetchMatchingDbSongs(conn, m_searchQuery);
  for (const auto &s : songs) {
    uris.push_back(s.songUri);
  }
  return uris;
}

} // namespace UI::Views

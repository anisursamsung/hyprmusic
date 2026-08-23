#include "QueueView.hpp"
#include "Dialogs/ActionMenuDialog.hpp"
#include "Dialogs/DialogCoordinator.hpp"
#include "Tabs/Shared/SongCard.hpp"
#include "Utils/IconProvider.hpp"
#include "Utils/ArtworkUtils.hpp"
#include <algorithm>
#include <cstring>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <unordered_set>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace {

struct QueueSongItem {
  int songId;
  unsigned songPos;
  std::string uri;
  std::string title;
  std::string artist;
};

// --- Helper: Parse & Resolve Queue Song Metadata ---

std::vector<QueueSongItem> fetchQueueSongs(struct mpd_connection *conn,
                                           Services::YtDlpService *ytDlpService,
                                           const std::string &searchQuery) {
  std::vector<QueueSongItem> queueSongs;
  if (!conn || !mpd_send_list_queue_meta(conn)) {
    return queueSongs;
  }

  struct mpd_song *s;
  while ((s = mpd_recv_song(conn)) != NULL) {
    int songId = mpd_song_get_id(s);
    unsigned songPos = mpd_song_get_pos(s);

    const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
    const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
    const char *uri = mpd_song_get_uri(s);

    std::string displayTitle;
    std::string displayArtist;

    std::string storedTitle, storedUploader;
    if (title && strlen(title) > 0) {
      displayTitle = std::string(title);
      displayArtist = artist ? artist : "Unavailable";
    } else if (uri && ytDlpService &&
               ytDlpService->getUrlTitle(uri, storedTitle, storedUploader)) {
      displayTitle = "Stream (" + storedTitle + ")";
      displayArtist = storedUploader.empty() ? "Unavailable" : storedUploader;
    } else if (uri) {
      std::string uriStr(uri);
      if (uriStr.find("googlevideo.com") != std::string::npos ||
          uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
        displayTitle = uriStr.length() > 50
                           ? Components::IconProvider::getIcon(
                                 Components::IconType::STREAM) +
                                 " Stream (" + uriStr.substr(0, 35) + "...)"
                           : uriStr;
      } else {
        displayTitle = uriStr;
      }
      displayArtist = "Unavailable";
    } else {
      displayTitle = "Unknown Track";
      displayArtist = "Unavailable";
    }

    if (!searchQuery.empty()) {
      std::string query = searchQuery;
      std::string searchTarget = displayTitle + " " + displayArtist;
      std::transform(searchTarget.begin(), searchTarget.end(),
                     searchTarget.begin(), ::tolower);
      std::transform(query.begin(), query.end(), query.begin(), ::tolower);
      if (searchTarget.find(query) == std::string::npos) {
        mpd_song_free(s);
        continue;
      }
    }

    queueSongs.push_back(
        {songId, songPos, uri ? uri : "", displayTitle, displayArtist});
    mpd_song_free(s);
  }
  mpd_response_finish(conn);
  return queueSongs;
}

// --- Helper: Render SongCard Component for Queue Track ---

std::shared_ptr<UI::Components::SongCard> renderQueueSongCard(
    const QueueViewContext &ctx, const QueueSongItem &item, bool isActive,
    int rounding, std::function<void(int)> onSongSelected) {
  int songId = item.songId;
  std::string indexStr = std::to_string(item.songPos + 1) + ". ";
  std::string songUriStr = item.uri;

  std::string cachedArt = Utils::getCachedTrackArtwork(songUriStr);
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
          .isActive = isActive,
          .onCardBodyClick =
              [ctx, songId, onSongSelected] {
                if (onSongSelected)
                  onSongSelected(songId);
                if (ctx.playMpdSongId)
                  ctx.playMpdSongId(songId);
              },
          .onActionClick =
              [ctx, songId, songUriStr, onSongSelected] {
                Dialogs::ActionMenuContext menuCtx{
                    .options = {Components::IconProvider::getIcon(
                                    Components::IconType::PLAY) +
                                    " Play",
                                Components::IconProvider::getIcon(
                                    Components::IconType::REMOVE) +
                                    " Remove from Queue",
                                Components::IconProvider::getIcon(
                                    Components::IconType::ADD_TO_PLAYLIST) +
                                    " Add to Playlist"},
                    .onSelect =
                        [ctx, songId, songUriStr,
                         onSongSelected](size_t idx, const std::string &) {
                          if (idx == 0) {
                            if (onSongSelected)
                              onSongSelected(songId);
                            if (ctx.playMpdSongId)
                              ctx.playMpdSongId(songId);
                          } else if (idx == 1) {
                            if (ctx.removeSongFromQueue)
                              ctx.removeSongFromQueue(songId);
                          } else if (idx == 2) {
                            if (!songUriStr.empty()) {
                              if (ctx.dialogCoordinator)
                                ctx.dialogCoordinator->showPlaylistSelectionDialog(songUriStr);
                              else if (ctx.showPlaylistSelectionDialog)
                                ctx.showPlaylistSelectionDialog(songUriStr);
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

// --- Helper: Action Menu for Top Bar (Play All, Shuffle Queue, Clear Queue) ---

void showQueueTopMenu(const QueueViewContext &ctx,
                      std::function<void()> onQueueCleared) {
  Dialogs::ActionMenuContext menuCtx{
      .options = {Components::IconProvider::getIcon(Components::IconType::PLAY) +
                      " Play All",
                  Components::IconProvider::getIcon(
                      Components::IconType::SHUFFLE) +
                      " Shuffle Queue",
                  Components::IconProvider::getIcon(
                      Components::IconType::CLEAR_ALL) +
                      " Clear Queue"},
      .onSelect =
          [ctx, onQueueCleared](size_t idx, const std::string &) {
            if (idx == 0) { // ▶ Play All
              ctx.runMpdCommand([](struct mpd_connection *conn) {
                if (conn)
                  mpd_run_play_pos(conn, 0);
              });
            } else if (idx == 1) { // 🔀 Shuffle Queue
              ctx.runMpdCommand([](struct mpd_connection *conn) {
                if (conn)
                  mpd_run_shuffle(conn);
              });
            } else if (idx == 2) { // 🗑️ Clear Queue
              if (ctx.dialogCoordinator) {
                ctx.dialogCoordinator->showClearQueueDialog(onQueueCleared);
              } else if (ctx.showClearQueueDialog) {
                ctx.showClearQueueDialog(onQueueCleared);
              } else {
                ctx.runMpdCommand([onQueueCleared](struct mpd_connection *conn) {
                  if (!conn)
                    return;
                  mpd_run_clear(conn);
                  if (onQueueCleared)
                    onQueueCleared();
                });
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
}

} // namespace

QueueView::QueueView(const QueueViewContext &ctx) : m_ctx(ctx) {}

void QueueView::rebuildUI(CSharedPointer<CRectangleElement> wrapper,
                          struct mpd_connection *conn, int activeSongId) {
  (void)conn;
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

  if (!m_queueContentLayout) {
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
            ->text("Queue")
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
                      struct mpd_status *status = mpd_run_status(conn);
                      int activeSongId = -1;
                      if (status) {
                        activeSongId = mpd_status_get_song_id(status);
                        mpd_status_free(status);
                      }
                      populateQueueSongs(conn, activeSongId);
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

    auto addItemBtn =
        CButtonBuilder::begin()
            ->label(
                Components::IconProvider::getIcon(Components::IconType::ADD) +
                " Add Item")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              if (m_ctx.showQueueAddItemDialog)
                m_ctx.showQueueAddItemDialog();
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
            ->commence();
    addItemBtn->setGrow(false);
    topSearchRow->addChild(addItemBtn);

    auto queueActionsBtn =
        CButtonBuilder::begin()
            ->label(Components::IconProvider::getIcon(Components::IconType::MENU))
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(Components::IconProvider::getCustomFontFamily())
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              showQueueTopMenu(m_ctx, [this]() {
                m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                  populateQueueSongs(conn, -1);
                });
              });
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {40.0F, 40.0F}))
            ->commence();
    queueActionsBtn->setGrow(false);
    topSearchRow->addChild(queueActionsBtn);

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

    m_queueContentLayout =
        CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_queueContentLayout->setMargin(5);

    scrollArea->addChild(m_queueContentLayout);
    tabMainLayout->addChild(scrollArea);
  }

  m_queueContentLayout->clearChildren();

  auto loadingText =
      CTextBuilder::begin()
          ->text(Components::IconProvider::getIcon(Components::IconType::LOADING) +
                 " Loading Queue...")
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
  m_queueContentLayout->addChild(loadingText);

  m_queueContentLayout->forceReposition();
  m_tabContentWrapper->forceReposition();

  m_ctx.backend->addTimer(
      std::chrono::milliseconds(1),
      [this, activeSongId](CAtomicSharedPointer<CTimer>, void *) {
        m_ctx.runMpdCommand([this, activeSongId](struct mpd_connection *conn) {
          populateQueueSongs(conn, activeSongId);
        });
      },
      nullptr);
}

void QueueView::setActiveSongId(int activeSongId) {
  m_lastActiveSongId = activeSongId;

  for (auto &pair : m_queueSongCards) {
    if (pair.second) {
      pair.second->setActive(activeSongId >= 0 && pair.first == activeSongId);
    }
  }
}

void QueueView::populateQueueSongs(struct mpd_connection *conn,
                                   int activeSongId) {
  m_lastActiveSongId = activeSongId;
  if (!m_queueContentLayout)
    return;

  m_queueContentLayout->clearChildren();
  m_queueSongCards.clear();

  auto palette = m_ctx.palette;
  int rounding = palette ? palette->m_vars.smallRounding : 5;
  std::string fontFamily = m_ctx.fontFamily;

  std::vector<QueueSongItem> queueSongs =
      fetchQueueSongs(conn, m_ctx.ytDlpService, m_searchQuery);
  std::vector<QueueSongItem> uncachedSongs;
  bool foundAny = !queueSongs.empty();

  for (const auto &item : queueSongs) {
    int songId = item.songId;
    std::string songUriStr = item.uri;
    bool isActive = (activeSongId >= 0 && songId == activeSongId);

    std::string cachedArt = Utils::getCachedTrackArtwork(songUriStr);
    bool isStreamUrl =
        (songUriStr.rfind("http://", 0) == 0 ||
         songUriStr.rfind("https://", 0) == 0 ||
         songUriStr.find("googlevideo.com") != std::string::npos ||
         songUriStr.find("youtube.com") != std::string::npos);

    if (cachedArt.empty() && !songUriStr.empty() && !isStreamUrl) {
      uncachedSongs.push_back(item);
    }

    auto card = renderQueueSongCard(m_ctx, item, isActive, rounding,
                                    [this](int id) { setActiveSongId(id); });
    m_queueSongCards[songId] = card;
    m_queueContentLayout->addChild(card->build());
  }

  // Non-blocking progressive micro-batched artwork resolution (2 tracks per 15ms batch for uncached items only)
  if (m_ctx.backend && m_ctx.runMpdCommand && !uncachedSongs.empty()) {
    auto stepState = std::make_shared<size_t>(0);
    auto processNextChunk = [this, uncachedSongs, stepState](auto self) -> void {
      if (*stepState >= uncachedSongs.size())
        return;
      m_ctx.runMpdCommand(
          [this, uncachedSongs, stepState, self](struct mpd_connection *conn) {
            size_t limit = std::min(*stepState + 2, uncachedSongs.size());
            for (size_t i = *stepState; i < limit; ++i) {
              const auto &item = uncachedSongs[i];
              std::string resolved = Utils::resolveTrackArtwork(conn, item.uri);
              if (!resolved.empty()) {
                auto it = m_queueSongCards.find(item.songId);
                if (it != m_queueSongCards.end() && it->second) {
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
        [processNextChunk](CAtomicSharedPointer<CTimer>, void *) {
          processNextChunk(processNextChunk);
        },
        nullptr);
  }

  if (!foundAny) {
    auto emptyText =
        CTextBuilder::begin()
            ->text(std::string("Queue is empty"))
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
    m_queueContentLayout->addChild(emptyText);
  }

  m_queueContentLayout->forceReposition();
  if (m_tabContentWrapper)
    m_tabContentWrapper->forceReposition();
}

} // namespace UI::Views

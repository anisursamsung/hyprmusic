#include "QueueView.hpp"
#include "../Dialogs/ActionMenuDialog.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unordered_set>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

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
    m_tabContentWrapper->addChild(tabMainLayout);

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
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
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
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
            ->commence();
    searchBar->setGrow(true);
    topSearchRow->addChild(searchBar);

    auto addItemBtn =
        CButtonBuilder::begin()
            ->label("➕ Add Item")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              if (m_ctx.showQueueAddItemDialog)
                m_ctx.showQueueAddItemDialog();
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
            ->commence();
    addItemBtn->setGrow(false);
    topSearchRow->addChild(addItemBtn);

    auto queueActionsBtn =
        CButtonBuilder::begin()
            ->label("⋮")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              Dialogs::showActionMenuDialog({
                  .options = {"▶ Play All", "🔀 Shuffle Queue", "🗑️ Clear Queue"},
                  .onSelect =
                      [this](size_t idx, const std::string &) {
                        if (idx == 0) { // ▶ Play All
                          m_ctx.runMpdCommand([](struct mpd_connection *conn) {
                            if (conn)
                              mpd_run_play_pos(conn, 0);
                          });
                        } else if (idx == 1) { // 🔀 Shuffle Queue
                          m_ctx.runMpdCommand([](struct mpd_connection *conn) {
                            if (conn)
                              mpd_run_shuffle(conn);
                          });
                        } else if (idx == 2) { // 🗑️ Clear Queue
                          m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                            if (!conn)
                              return;
                            mpd_run_clear(conn);
                            populateQueueSongs(conn, -1);
                          });
                        }
                      },
                  .parentWindow = m_ctx.window,
                  .backend = m_ctx.backend,
                  .palette = m_ctx.palette,
                  .fontFamily = m_ctx.fontFamily});
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {32.0F, 32.0F}))
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
            ->gap(10)
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
          ->text(std::string("⏳ Loading Queue..."))
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

void QueueView::populateQueueSongs(struct mpd_connection *conn, int activeSongId) {
  if (activeSongId >= 0) {
    m_lastActiveSongId = activeSongId;
  }
  if (!m_queueContentLayout)
    return;

  m_queueContentLayout->clearChildren();
  m_queueSongTexts.clear();

  auto palette = m_ctx.palette;
  int rounding = palette ? palette->m_vars.smallRounding : 5;
  std::string fontFamily = m_ctx.fontFamily;

  if (!conn || !mpd_send_list_queue_meta(conn)) {
    std::cerr << "MPD: Failed to send list queue command" << std::endl;
    return;
  }

  bool foundAny = false;
  struct mpd_song *s;
  std::unordered_set<int> currentQueueIds;
  while ((s = mpd_recv_song(conn)) != NULL) {
    int songId = mpd_song_get_id(s);
    currentQueueIds.insert(songId);
    unsigned songPos = mpd_song_get_pos(s);

    const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
    const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
    const char *uri = mpd_song_get_uri(s);
    std::string displayTitle;

    std::string storedTitle, storedUploader;
    if (title && strlen(title) > 0) {
      std::string artistStr = artist ? artist : "Unknown Artist";
      displayTitle = std::string(title) + " - " + artistStr;
    } else if (uri && m_ctx.ytDlpService &&
               m_ctx.ytDlpService->getUrlTitle(uri, storedTitle, storedUploader)) {
      displayTitle = "Stream (" + storedTitle + ")";
      if (!storedUploader.empty()) {
        displayTitle += " - " + storedUploader;
      }
    } else if (uri) {
      std::string uriStr(uri);
      if (uriStr.find("googlevideo.com") != std::string::npos ||
          uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
        if (uriStr.length() > 50) {
          displayTitle = "🌐 Stream (" + uriStr.substr(0, 35) + "...)";
        } else {
          displayTitle = uriStr;
        }
      } else {
        displayTitle = uriStr;
      }
    } else {
      displayTitle = "Unknown Track";
    }

    if (!m_searchQuery.empty()) {
      std::string query = m_searchQuery;
      std::string titleLower = displayTitle;
      std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
      std::transform(query.begin(), query.end(), query.begin(), ::tolower);
      if (titleLower.find(query) == std::string::npos) {
        mpd_song_free(s);
        continue;
      }
    }

    foundAny = true;
    auto songItem = CRectangleBuilder::begin()
                        ->color([palette] {
                          return palette ? palette->m_colors.base
                                         : CHyprColor(0.15, 0.15, 0.15, 1.0);
                        })
                        ->rounding(rounding)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {1.0F, 40.0F}))
                        ->commence();
    songItem->setReceivesMouse(true);
    songItem->setMouseButton(
        [this, songId](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (m_ctx.playMpdSongId)
              m_ctx.playMpdSongId(songId);
          }
        });

    auto rowLayout =
        CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    rowLayout->setMargin(6);

    std::string indexStr = std::to_string(songPos + 1) + ". ";
    auto songText =
        CTextBuilder::begin()
            ->text(std::string(indexStr + displayTitle))
            ->color([this, songId] {
              auto palette = m_ctx.palette;
              bool isPlaying = (songId == m_lastActiveSongId);
              if (isPlaying) {
                return palette ? palette->m_colors.accent
                               : CHyprColor(0.2, 0.8, 0.4, 1.0);
              }
              return palette ? palette->m_colors.text
                             : CHyprColor(1, 1, 1, 1);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->align(HT_FONT_ALIGN_LEFT)
            ->noEllipsize(false)
            ->interactable(true)
            ->commence();
    songText->setReceivesMouse(true);
    songText->setMouseButton(
        [this, songId](Input::eMouseButton button, bool down) {
          if (button == Input::MOUSE_BUTTON_LEFT && !down) {
            if (m_ctx.playMpdSongId)
              m_ctx.playMpdSongId(songId);
          }
        });
    m_queueSongTexts[songId] = songText;

    std::string songUriStr = uri ? uri : "";
    std::string songTitleStr = displayTitle;
    auto actionBtn =
        CButtonBuilder::begin()
            ->label("⋮")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this, songId, songUriStr, songTitleStr](CSharedPointer<CButtonElement>) {
              Dialogs::showActionMenuDialog({
                  .options = {"▶ Play", "🗑️ Remove from Queue", "📁 Add to Playlist"},
                  .onSelect =
                      [this, songId, songUriStr](size_t idx, const std::string &) {
                        if (idx == 0) { // ▶ Play
                          if (m_ctx.playMpdSongId)
                            m_ctx.playMpdSongId(songId);
                        } else if (idx == 1) { // 🗑️ Remove
                          if (m_ctx.removeSongFromQueue)
                            m_ctx.removeSongFromQueue(songId);
                        } else if (idx == 2) { // 📁 Add to Playlist
                          if (!songUriStr.empty() && m_ctx.showPlaylistSelectionDialog) {
                            m_ctx.showPlaylistSelectionDialog(songUriStr);
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

    auto textContainer =
        CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_PERCENT, {1.0F, 1.0F}))
            ->commence();
    textContainer->setGrow(true);
    textContainer->addChild(songText);
    rowLayout->addChild(textContainer);

    songItem->addChild(rowLayout);
    m_queueContentLayout->addChild(songItem);

    mpd_song_free(s);
  }
  mpd_response_finish(conn);

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

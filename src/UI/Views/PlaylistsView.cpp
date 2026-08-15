#include "PlaylistsView.hpp"
#include "../Components/IconProvider.hpp"
#include "../Components/SongCard.hpp"
#include "../Components/UIFactory.hpp"
#include "../Dialogs/ActionMenuDialog.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <unordered_set>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

PlaylistsView::PlaylistsView(const PlaylistsViewContext &ctx) : m_ctx(ctx) {}

void PlaylistsView::layoutPlaylists() {
  if (!m_leftItemsLayout)
    return;

  m_leftItemsLayout->clearChildren();

  auto palette = m_ctx.palette;
  int rounding = palette ? palette->m_vars.smallRounding : 5;
  std::string fontFamily = m_ctx.fontFamily;

  if (m_currentPlaylists.empty()) {
    auto emptyText =
        CTextBuilder::begin()
            ->text(std::string("No playlists found"))
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
    m_leftItemsLayout->addChild(emptyText);
  } else {
    double parentWidth = m_leftItemsLayout->size().x;
    if (parentWidth <= 0) {
      parentWidth = 800.0;
    }

    double minPercent = 0.10;
    double gap = 15.0;
    double minSize = std::max(parentWidth * minPercent, 130.0);

    size_t columns = std::max(
        (size_t)1, (size_t)std::floor((parentWidth + gap) / (minSize + gap)));
    double actualSize = (parentWidth - (columns - 1) * gap) / columns;
    if (actualSize < 10)
      actualSize = 130.0;

    CSharedPointer<CRowLayoutElement> currentRow = nullptr;

    for (size_t i = 0; i < m_currentPlaylists.size(); ++i) {
      if (i % columns == 0) {
        currentRow =
            CRowLayoutBuilder::begin()
                ->gap((size_t)gap)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                ->commence();
        m_leftItemsLayout->addChild(currentRow);
      }

      auto plName = m_currentPlaylists[i];
      bool isSelected = (plName == m_selectedPlaylist);

      auto card =
          CRectangleBuilder::begin()
              ->color([palette, isSelected] {
                if (isSelected) {
                  return palette ? palette->m_colors.text
                                 : CHyprColor(1.0, 1.0, 1.0, 1.0);
                }
                return palette ? palette->m_colors.alternateBase
                               : CHyprColor(0.18, 0.18, 0.18, 1.0);
              })
              ->rounding(rounding)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {(float)actualSize, (float)actualSize}))
              ->commence();

      auto textCol =
          CColumnLayoutBuilder::begin()
              ->gap(4)
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {0.85F, 1.0F}))
              ->commence();

      auto titleText =
          CTextBuilder::begin()
              ->text(std::string(plName))
              ->color([palette, isSelected] {
                if (isSelected) {
                  return palette ? palette->m_colors.alternateBase
                                 : CHyprColor(0.18, 0.18, 0.18, 1.0);
                }
                return palette ? palette->m_colors.text
                               : CHyprColor(1.0, 1.0, 1.0, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->align(HT_FONT_ALIGN_CENTER)
              ->noEllipsize(false)
              ->commence();

      int trackCount = 0;
      if (m_playlistTrackCounts.find(plName) != m_playlistTrackCounts.end()) {
        trackCount = m_playlistTrackCounts[plName];
      }
      std::string subStr = "(" + std::to_string(trackCount) +
                           (trackCount == 1 ? " track)" : " tracks)");

      auto trackCountText =
          CTextBuilder::begin()
              ->text(std::move(subStr))
              ->color([palette, isSelected] {
                if (isSelected) {
                  auto altBase = palette ? palette->m_colors.alternateBase
                                         : CHyprColor(0.18, 0.18, 0.18, 1.0);
                  auto acc = palette ? palette->m_colors.text
                                     : CHyprColor(1.0, 1.0, 1.0, 1.0);
                  return altBase.mix(acc, 0.25);
                }
                auto txt = palette ? palette->m_colors.text
                                   : CHyprColor(1.0, 1.0, 1.0, 1.0);
                auto altBase = palette ? palette->m_colors.alternateBase
                                       : CHyprColor(0.18, 0.18, 0.18, 1.0);
                return txt.mix(altBase, 0.35);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->align(HT_FONT_ALIGN_CENTER)
              ->noEllipsize(true)
              ->commence();

      textCol->addChild(titleText);
      textCol->addChild(trackCountText);

      textCol->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
      textCol->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);
      textCol->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);
      card->addChild(textCol);

      card->setReceivesMouse(true);
      card->setMouseButton(
          [this, plName](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && !down) {
              m_selectedPlaylist = plName;
              m_detailedView = true;
              m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                rebuildUI(m_tabContentWrapper, conn);
              });
            }
          });

      auto actionBtn =
          CButtonBuilder::begin()
              ->label("⋮")
              ->alignText(HT_FONT_ALIGN_CENTER)
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->onMainClick([this, plName](CSharedPointer<CButtonElement>) {
                Dialogs::showActionMenuDialog({
                    .options = {Components::IconProvider::getIcon(Components::IconType::PLAY) + " Play",
                                Components::IconProvider::getIcon(Components::IconType::ADD) + " Add to Queue",
                                Components::IconProvider::getIcon(Components::IconType::EDIT) + " Rename",
                                Components::IconProvider::getIcon(Components::IconType::REMOVE) + " Delete"},
                    .onSelect =
                        [this, plName](size_t idx, const std::string &) {
                          if (idx == 0) { // ▶ Play
                            m_ctx.runMpdCommand([plName](struct mpd_connection *conn) {
                              mpd_run_clear(conn);
                              mpd_run_load(conn, plName.c_str());
                              mpd_run_play(conn);
                            });
                            if (m_ctx.showNotification)
                              m_ctx.showNotification("Playing " + plName);
                            if (m_ctx.updateStatus)
                              m_ctx.updateStatus();
                          } else if (idx == 1) { // ➕ Add to Queue
                            addPlaylistToQueue(plName);
                          } else if (idx == 2) { // ✏️ Rename
                            if (m_ctx.showRenameDialog)
                              m_ctx.showRenameDialog(plName);
                          } else if (idx == 3) { // 🗑️ Delete
                            m_ctx.runMpdCommand([plName](struct mpd_connection *conn) {
                              mpd_run_playlist_clear(conn, plName.c_str());
                              mpd_run_rm(conn, plName.c_str());
                            });
                            if (m_selectedPlaylist == plName) {
                              m_selectedPlaylist = "";
                              m_detailedView = false;
                            }
                            if (m_ctx.showNotification)
                              m_ctx.showNotification("Deleted " + plName);
                            m_ctx.backend->addTimer(
                                std::chrono::milliseconds(100),
                                [this](CAtomicSharedPointer<CTimer>, void *) {
                                  m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                                    rebuildUI(m_tabContentWrapper, conn);
                                  });
                                },
                                nullptr);
                          }
                        },
                    .parentWindow = m_ctx.window,
                    .backend = m_ctx.backend,
                    .palette = m_ctx.palette,
                    .fontFamily = m_ctx.fontFamily});
              })
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                  CDynamicSize::HT_SIZE_ABSOLUTE,
                                  {28.0F, 24.0F}))
              ->commence();

      actionBtn->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
      actionBtn->setPositionFlag(IElement::HT_POSITION_FLAG_RIGHT, true);
      actionBtn->setPositionFlag(IElement::HT_POSITION_FLAG_TOP, true);
      actionBtn->setAbsolutePosition(Hyprutils::Math::Vector2D(-8.0, 8.0));
      card->addChild(actionBtn);

      currentRow->addChild(card);
    }
  }

  m_leftItemsLayout->forceReposition();
}

void PlaylistsView::rebuildLeftItems(struct mpd_connection *conn) {
  if (!m_leftItemsLayout)
    return;

  m_currentPlaylists.clear();
  m_playlistTrackCounts.clear();

  if (conn && mpd_send_list_playlists(conn)) {
    struct mpd_playlist *pl;
    while ((pl = mpd_recv_playlist(conn)) != NULL) {
      const char *name = mpd_playlist_get_path(pl);
      if (name) {
        std::string sName(name);
        if (!m_searchQuery.empty()) {
          std::string query = m_searchQuery;
          std::string plLower = sName;
          std::transform(plLower.begin(), plLower.end(), plLower.begin(), ::tolower);
          std::transform(query.begin(), query.end(), query.begin(), ::tolower);
          if (plLower.find(query) == std::string::npos) {
            mpd_playlist_free(pl);
            continue;
          }
        }
        m_currentPlaylists.push_back(sName);
      }
      mpd_playlist_free(pl);
    }
    mpd_response_finish(conn);
  }

  for (const auto &plName : m_currentPlaylists) {
    int count = 0;
    if (conn && mpd_send_list_playlist_meta(conn, plName.c_str())) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        count++;
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }
    m_playlistTrackCounts[plName] = count;
  }

  layoutPlaylists();
}

void PlaylistsView::rebuildRightItems(struct mpd_connection *conn) {
  if (!m_rightItemsLayout)
    return;

  m_rightItemsLayout->clearChildren();

  auto palette = m_ctx.palette;
  int rounding = palette ? palette->m_vars.smallRounding : 5;
  std::string fontFamily = m_ctx.fontFamily;

  if (m_selectedPlaylist.empty() || !m_detailedView) {
    auto promptText =
        CTextBuilder::begin()
            ->text(std::string("Select a playlist to view details"))
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
    m_rightItemsLayout->addChild(promptText);
    m_rightItemsLayout->forceReposition();
    return;
  }

  std::string plName = m_selectedPlaylist;

  struct PlaylistSongItem {
    int songPos;
    std::string uri;
    std::string title;
    std::string artist;
  };
  std::vector<PlaylistSongItem> playlistSongs;

  bool foundAny = false;
  if (conn && mpd_send_list_playlist_meta(conn, plName.c_str())) {
    struct mpd_song *s;
    int songPos = 0;
    while ((s = mpd_recv_song(conn)) != NULL) {
      const char *artist = mpd_song_get_tag(s, MPD_TAG_ARTIST, 0);
      const char *title = mpd_song_get_tag(s, MPD_TAG_TITLE, 0);
      const char *uri = mpd_song_get_uri(s);

      // ── Resolve display title & artist ───────────────────────────────────
      std::string displayTitle;
      std::string displayArtist;
      std::string storedTitle, storedUploader;

      if (title && strlen(title) > 0) {
        displayTitle  = std::string(title);
        displayArtist = artist ? std::string(artist) : "Unknown Artist";
      } else if (uri && m_ctx.ytDlpService &&
                 m_ctx.ytDlpService->getUrlTitle(uri, storedTitle, storedUploader)) {
        displayTitle  = "Stream (" + storedTitle + ")";
        displayArtist = storedUploader.empty() ? "Unavailable" : storedUploader;
      } else if (uri) {
        std::string uriStr(uri);
        if (uriStr.find("googlevideo.com") != std::string::npos ||
            uriStr.find("http://") == 0 || uriStr.find("https://") == 0) {
          displayTitle = uriStr.length() > 50
                             ? "\U0001f310 Stream (" + uriStr.substr(0, 35) + "...)"
                             : uriStr;
        } else {
          displayTitle = uriStr;
        }
        displayArtist = "Unavailable";
      } else {
        displayTitle  = "Unknown Track";
        displayArtist = "Unavailable";
      }

      playlistSongs.push_back({songPos++, uri ? uri : "", displayTitle, displayArtist});
      mpd_song_free(s);
    }
    mpd_response_finish(conn);

    std::vector<PlaylistSongItem> uncachedSongs;

    m_playlistSongCards.clear();
    for (const auto &item : playlistSongs) {
      foundAny = true;
      std::string songUriStr = item.uri;
      std::string indexStr  = std::to_string(item.songPos + 1) + ". ";
      int currentPos        = item.songPos;

      std::string cachedArt = Utils::getCachedTrackArtwork(songUriStr);
      std::string artPath = cachedArt.empty() ? Utils::getDefaultArtworkPath() : cachedArt;

      if (cachedArt.empty() && !songUriStr.empty()) {
        uncachedSongs.push_back(item);
      }

      // ── Build card via reusable SongCard component ────────────────────────
      auto card = std::make_shared<UI::Components::SongCard>(
          UI::Components::SongCardConfig{
              .palette    = palette,
              .fontFamily = fontFamily,
              .rounding   = rounding,
              .cardHeight = 70.0f,
              .title      = indexStr + item.title,
              .subtitle   = item.artist,
              .imagePath  = artPath,
              .isActive   = false,
              .onCardBodyClick = [this, songUriStr] {
                if (!songUriStr.empty() && m_ctx.playSongFromUri)
                  m_ctx.playSongFromUri(songUriStr);
              },
              .onActionClick = [this, plName, currentPos, songUriStr] {
                Dialogs::showActionMenuDialog({
                    .options  = {Components::IconProvider::getIcon(Components::IconType::PLAY) + " Play",
                                 Components::IconProvider::getIcon(Components::IconType::ADD) + " Add to Queue",
                                 Components::IconProvider::getIcon(Components::IconType::COPY) + " Copy to Playlist",
                                 Components::IconProvider::getIcon(Components::IconType::MOVE) + " Move to Playlist",
                                 Components::IconProvider::getIcon(Components::IconType::REMOVE) + " Remove"},
                    .onSelect =
                        [this, plName, currentPos,
                         songUriStr](size_t idx, const std::string &) {
                          if (idx == 0) {
                            if (!songUriStr.empty() && m_ctx.playSongFromUri)
                              m_ctx.playSongFromUri(songUriStr);
                          } else if (idx == 1) {
                            if (!songUriStr.empty() && m_ctx.addSongToQueue)
                              m_ctx.addSongToQueue(songUriStr);
                          } else if (idx == 2) {
                            if (!songUriStr.empty() &&
                                m_ctx.showPlaylistSelectionDialog)
                              m_ctx.showPlaylistSelectionDialog(songUriStr, -1);
                          } else if (idx == 3) {
                            if (!songUriStr.empty() &&
                                m_ctx.showPlaylistSelectionDialog)
                              m_ctx.showPlaylistSelectionDialog(songUriStr,
                                                                currentPos);
                          } else if (idx == 4) {
                            m_ctx.runMpdCommand(
                                [plName,
                                 currentPos](struct mpd_connection *conn) {
                                  mpd_run_playlist_delete(
                                      conn, plName.c_str(), currentPos);
                                });
                            if (m_ctx.showNotification)
                              m_ctx.showNotification("Removed from " + plName);
                            m_ctx.backend->addTimer(
                                std::chrono::milliseconds(100),
                                [this](CAtomicSharedPointer<CTimer>, void *) {
                                  m_ctx.runMpdCommand(
                                      [this](struct mpd_connection *conn) {
                                        rebuildRightItems(conn);
                                      });
                                },
                                nullptr);
                          }
                        },
                    .parentWindow = m_ctx.window,
                    .backend      = m_ctx.backend,
                    .palette      = m_ctx.palette,
                    .fontFamily   = m_ctx.fontFamily});
              }});
      m_playlistSongCards[currentPos] = card;
      m_rightItemsLayout->addChild(card->build());
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
            std::string resolved = Utils::resolveTrackArtwork(conn, item.uri);
            if (!resolved.empty()) {
              auto it = m_playlistSongCards.find(item.songPos);
              if (it != m_playlistSongCards.end() && it->second) {
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
  }

  if (!foundAny) {
    auto emptyText =
        CTextBuilder::begin()
            ->text(std::string("Playlist is empty"))
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
    m_rightItemsLayout->addChild(emptyText);
  }

  m_rightItemsLayout->forceReposition();
}

void PlaylistsView::rebuildUI(CSharedPointer<CRectangleElement> wrapper,
                              struct mpd_connection *conn) {
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  m_tabContentWrapper->clearChildren();

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;

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
          ->text("Playlists")
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

  if (!m_detailedView || m_selectedPlaylist.empty()) {
    // ==========================================
    // GRID VIEW (Grid of Playlist Cards)
    // ==========================================
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
            ->placeholder("Search playlists...")
            ->defaultText(std::string(m_searchQuery))
            ->onTextEdited([this](CSharedPointer<CTextboxElement>,
                                  const std::string &text) {
              m_searchQuery = text;
              m_ctx.backend->addTimer(
                  std::chrono::milliseconds(1),
                  [this](CAtomicSharedPointer<CTimer>, void *) {
                    m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                      rebuildLeftItems(conn);
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

    auto createPlBtn =
        CButtonBuilder::begin()
            ->label(Components::IconProvider::getIcon(Components::IconType::ADD) + " Create Playlist")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              if (m_ctx.showCreatePlaylistDialog)
                m_ctx.showCreatePlaylistDialog();
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
            ->commence();
    createPlBtn->setGrow(false);
    topSearchRow->addChild(createPlBtn);

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

    m_leftItemsLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_leftItemsLayout->setMargin(5);
    scrollArea->addChild(m_leftItemsLayout);
    tabMainLayout->addChild(scrollArea);

    m_rightItemsLayout = nullptr;
    rebuildLeftItems(conn);
  } else {
    // ==========================================
    // DETAILED VIEW (Tracks inside selected playlist)
    // ==========================================
    std::string plName = m_selectedPlaylist;

    auto topHeaderCol =
        CColumnLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    topHeaderCol->setMargin(10);

    auto plTitleRow =
        CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 35.0F}))
            ->commence();

    auto backBtn =
        CButtonBuilder::begin()
            ->label("◀ Back")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
              m_detailedView = false;
              m_selectedPlaylist = "";
              m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                rebuildUI(m_tabContentWrapper, conn);
              });
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
            ->commence();
    backBtn->setGrow(false);
    plTitleRow->addChild(backBtn);

    auto plTitle =
        CTextBuilder::begin()
            ->text(std::string(Components::IconProvider::getIcon(Components::IconType::FOLDER) + " " + plName))
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H2))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->align(HT_FONT_ALIGN_LEFT)
            ->commence();
    plTitle->setGrow(true);
    plTitleRow->addChild(plTitle);

    auto addTrackBtn =
        CButtonBuilder::begin()
            ->label(Components::IconProvider::getIcon(Components::IconType::ADD) + " Add Item")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this, plName](CSharedPointer<CButtonElement>) {
              if (m_ctx.showPlaylistAddItemDialog)
                m_ctx.showPlaylistAddItemDialog(plName);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
            ->commence();
    addTrackBtn->setGrow(false);
    plTitleRow->addChild(addTrackBtn);

    auto actionBtn =
        CButtonBuilder::begin()
            ->label("⋮")
            ->alignText(HT_FONT_ALIGN_CENTER)
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
            ->onMainClick([this, plName](CSharedPointer<CButtonElement>) {
              Dialogs::showActionMenuDialog({
                  .options = {Components::IconProvider::getIcon(Components::IconType::PLAY) + " Play",
                              Components::IconProvider::getIcon(Components::IconType::ADD) + " Add to Queue",
                              Components::IconProvider::getIcon(Components::IconType::EDIT) + " Rename",
                              Components::IconProvider::getIcon(Components::IconType::REMOVE) + " Delete"},
                  .onSelect =
                      [this, plName](size_t idx, const std::string &) {
                        if (idx == 0) { // ▶ Play
                          m_ctx.runMpdCommand([plName](struct mpd_connection *conn) {
                            mpd_run_clear(conn);
                            mpd_run_load(conn, plName.c_str());
                            mpd_run_play(conn);
                          });
                          if (m_ctx.showNotification)
                            m_ctx.showNotification("Playing " + plName);
                          if (m_ctx.updateStatus)
                            m_ctx.updateStatus();
                        } else if (idx == 1) { // ➕ Add to Queue
                          addPlaylistToQueue(plName);
                        } else if (idx == 2) { // ✏️ Rename
                          if (m_ctx.showRenameDialog)
                            m_ctx.showRenameDialog(plName);
                        } else if (idx == 3) { // 🗑️ Delete
                          m_ctx.runMpdCommand([plName](struct mpd_connection *conn) {
                            mpd_run_playlist_clear(conn, plName.c_str());
                            mpd_run_rm(conn, plName.c_str());
                          });
                          m_selectedPlaylist = "";
                          m_detailedView = false;
                          if (m_ctx.showNotification)
                            m_ctx.showNotification("Deleted " + plName);
                          m_ctx.backend->addTimer(
                              std::chrono::milliseconds(100),
                              [this](CAtomicSharedPointer<CTimer>, void *) {
                                m_ctx.runMpdCommand([this](struct mpd_connection *conn) {
                                  rebuildUI(m_tabContentWrapper, conn);
                                });
                              },
                              nullptr);
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
    actionBtn->setGrow(false);
    plTitleRow->addChild(actionBtn);

    topHeaderCol->addChild(plTitleRow);
    tabMainLayout->addChild(topHeaderCol);

    auto scrollArea =
        CScrollAreaBuilder::begin()
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 1.0F}))
            ->scrollY(true)
            ->commence();
    scrollArea->setGrow(true);

    m_rightItemsLayout =
        CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    m_rightItemsLayout->setMargin(5);
    scrollArea->addChild(m_rightItemsLayout);
    tabMainLayout->addChild(scrollArea);

    m_leftItemsLayout = nullptr;
    rebuildRightItems(conn);
  }

  m_tabContentWrapper->forceReposition();
}

void PlaylistsView::addPlaylistToQueue(const std::string &plName) {
  if (plName.empty())
    return;

  m_ctx.runMpdCommand([this, plName](struct mpd_connection *conn) {
    if (!conn)
      return;

    // 1. Get current queue URIs
    std::unordered_set<std::string> queueUris;
    if (mpd_send_list_queue_meta(conn)) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        const char *uri = mpd_song_get_uri(s);
        if (uri)
          queueUris.insert(std::string(uri));
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }

    // 2. Get tracks in playlist
    std::vector<std::string> plTracks;
    if (mpd_send_list_playlist_meta(conn, plName.c_str())) {
      struct mpd_song *s;
      while ((s = mpd_recv_song(conn)) != NULL) {
        const char *uri = mpd_song_get_uri(s);
        if (uri)
          plTracks.push_back(std::string(uri));
        mpd_song_free(s);
      }
      mpd_response_finish(conn);
    }

    // 3. Add non-duplicate tracks to queue
    int addedCount = 0;
    int skippedCount = 0;
    for (const auto &uri : plTracks) {
      if (queueUris.find(uri) == queueUris.end()) {
        if (mpd_run_add(conn, uri.c_str())) {
          queueUris.insert(uri);
          addedCount++;
        }
      } else {
        skippedCount++;
      }
    }

    if (m_ctx.showNotification) {
      if (addedCount > 0 && skippedCount > 0) {
        m_ctx.showNotification("Added " + std::to_string(addedCount) +
                                " tracks from " + plName + " (" +
                                std::to_string(skippedCount) +
                                " skipped as duplicates)");
      } else if (addedCount > 0) {
        m_ctx.showNotification("Added " + std::to_string(addedCount) +
                                " tracks from " + plName + " to queue");
      } else if (skippedCount > 0) {
        m_ctx.showNotification("All tracks from " + plName +
                                " already in queue");
      } else {
        m_ctx.showNotification("Playlist " + plName + " is empty");
      }
    }
  });

  if (m_ctx.updateStatus)
    m_ctx.updateStatus();
}

} // namespace UI::Views

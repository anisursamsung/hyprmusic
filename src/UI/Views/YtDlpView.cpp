#include "YtDlpView.hpp"
#include "../Components/SongCard.hpp"
#include "../Dialogs/ActionMenuDialog.hpp"
#include "../Dialogs/DownloadProgressDialog.hpp"
#include "../../Utils/ArtworkUtils.hpp"
#include "../../Utils/ClipboardUtils.hpp"
#include "../../Utils/FormatUtils.hpp"
#include "../../Utils/StreamUtils.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <iostream>
#include <thread>

namespace UI::Views {

YtDlpView::YtDlpView(const YtDlpViewContext &ctx) : m_ctx(ctx) {}

void YtDlpView::triggerSearch() {
  if (m_searchTitle.empty()) {
    if (m_ctx.showNotification)
      m_ctx.showNotification("Please enter a YouTube search term or URL");
    return;
  }

  m_searching = true;
  m_ytdlpNeedRebuild = true;

  if (m_ctx.updateStatus)
    m_ctx.updateStatus();

  std::string query = m_searchTitle;
  std::string countStr = m_resultCount;
  int count = 5;
  try {
    count = std::stoi(countStr);
  } catch (...) {
    count = 5;
  }

  if (m_ctx.ytDlpService) {
    m_ctx.ytDlpService->triggerSearch(
        query, count,
        [this](const std::vector<Services::YtDlpResult> &results, bool isPlaylist,
               const std::string &plTitle, const std::string &plId) {
          if (m_ctx.backend) {
            m_ctx.backend->addTimer(
                std::chrono::milliseconds(1),
                [this, results, isPlaylist, plTitle, plId](CAtomicSharedPointer<CTimer>, void *) {
                  m_searching = false;
                  m_results = results;
                  m_isPlaylist = isPlaylist;
                  m_playlistTitle = plTitle;
                  m_playlistId = plId;
                  m_ytdlpNeedRebuild = true;

                  if (m_ctx.showNotification) {
                    if (results.empty()) {
                      m_ctx.showNotification("No YouTube results found");
                    } else {
                      m_ctx.showNotification("Found " + std::to_string(results.size()) +
                                             " YouTube tracks");
                    }
                  }

                  if (m_ctx.updateStatus)
                    m_ctx.updateStatus();
                },
                nullptr);
          }
        });
  }
}

void YtDlpView::loadYoutubePlaylist(
    const std::vector<Services::YtDlpResult> &tracks,
    const std::string &saveToPlaylistName, bool startPlaying) {
  if (tracks.empty())
    return;

  std::string displayPlName = saveToPlaylistName;
  if (!saveToPlaylistName.empty()) {
    displayPlName = Utils::sanitizePlaylistName(saveToPlaylistName);
  }

  std::thread([this, tracks, startPlaying, displayPlName]() {
    m_ctx.backend->addTimer(
        std::chrono::milliseconds(1),
        [this, displayPlName, tracks](CAtomicSharedPointer<CTimer>, void *) {
          if (m_ctx.showNotification) {
            if (!displayPlName.empty()) {
              m_ctx.showNotification("⏳ Saving YouTube playlist '" + displayPlName + "'...");
            } else {
              m_ctx.showNotification("⏳ Resolving YouTube playlist (" +
                                      std::to_string(tracks.size()) + " tracks)...");
            }
          }
        },
        nullptr);

    if (startPlaying && displayPlName.empty()) {
      m_ctx.runMpdCommand([](struct mpd_connection *conn) { mpd_run_clear(conn); });
    }

    for (size_t i = 0; i < tracks.size(); ++i) {
      const auto &t = tracks[i];
      std::string realUrl = extractDirectStreamUrl(t.url);
      if (realUrl.empty())
        continue;

      std::string resTitle = t.title;
      std::string resUploader = t.uploader;

      if (m_ctx.ytDlpService) {
        m_ctx.ytDlpService->setUrlTitle(realUrl, resTitle, resUploader);
      }

      if (!displayPlName.empty()) {
        m_ctx.runMpdCommand([displayPlName, realUrl](struct mpd_connection *conn) {
          mpd_run_playlist_add(conn, displayPlName.c_str(), realUrl.c_str());
        });
      } else {
        m_ctx.runMpdCommand([realUrl](struct mpd_connection *conn) {
          mpd_run_add(conn, realUrl.c_str());
        });
      }

      if (startPlaying && i == 0 && displayPlName.empty()) {
        m_ctx.runMpdCommand([](struct mpd_connection *conn) { mpd_run_play(conn); });
      }
    }

    m_ctx.backend->addTimer(
        std::chrono::milliseconds(1),
        [this, displayPlName, startPlaying](CAtomicSharedPointer<CTimer>, void *) {
          if (m_ctx.showNotification) {
            if (!displayPlName.empty()) {
              m_ctx.showNotification("✅ Playlist '" + displayPlName + "' saved!");
            } else {
              m_ctx.showNotification("✅ YouTube playlist added to Queue!");
            }
          }
          if (m_ctx.updateStatus)
            m_ctx.updateStatus();
        },
        nullptr);
  }).detach();
}

void YtDlpView::rebuildUI(CSharedPointer<CRectangleElement> wrapper,
                          struct mpd_connection *conn) {
  (void)conn;
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
          ->text("YT-DLP")
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
          ->gap(8)
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

  auto titleInput =
      CTextboxBuilder::begin()
          ->placeholder("Search YouTube")
          ->defaultText(std::string(m_searchTitle))
          ->onTextEdited([this](CSharedPointer<CTextboxElement>, const std::string &text) {
            m_searchTitle = text;
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 32.0F}))
          ->commence();
  titleInput->setGrow(true);
  topSearchRow->addChild(titleInput);

  auto pasteBtn = CButtonBuilder::begin()
                      ->label("📋")
                      ->alignText(HT_FONT_ALIGN_CENTER)
                      ->fontFamily(std::string(fontFamily))
                      ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                      ->onMainClick([this, titleInput](CSharedPointer<CButtonElement>) {
                        std::string pasted = Utils::readFromClipboard();
                        if (!pasted.empty()) {
                          titleInput->rebuild()->defaultText(std::string(pasted))->commence();
                          m_searchTitle = pasted;
                        }
                      })
                      ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                          CDynamicSize::HT_SIZE_ABSOLUTE,
                                          {32.0F, 32.0F}))
                      ->commence();
  pasteBtn->setGrow(false);
  topSearchRow->addChild(pasteBtn);

  auto countLabel = CTextBuilder::begin()
                        ->text(std::string("Count:"))
                        ->color([palette] {
                          return palette ? palette->m_colors.text
                                         : CHyprColor(0.8, 0.8, 0.8, 1.0);
                        })
                        ->fontFamily(std::string(fontFamily))
                        ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                            CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
                        ->commence();
  countLabel->setGrow(false);
  topSearchRow->addChild(countLabel);

  auto countInput =
      CTextboxBuilder::begin()
          ->placeholder("5")
          ->defaultText(std::string(m_resultCount))
          ->onTextEdited([this](CSharedPointer<CTextboxElement>, const std::string &text) {
            m_resultCount = text;
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {45.0F, 32.0F}))
          ->commence();
  countInput->setGrow(false);
  topSearchRow->addChild(countInput);

  auto submitBtn = CButtonBuilder::begin()
                       ->label("🔍 Search")
                       ->alignText(HT_FONT_ALIGN_CENTER)
                       ->fontFamily(std::string(fontFamily))
                       ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                       ->onMainClick([this](CSharedPointer<CButtonElement>) { triggerSearch(); })
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                           CDynamicSize::HT_SIZE_ABSOLUTE, {95.0F, 32.0F}))
                       ->commence();
  submitBtn->setGrow(false);
  topSearchRow->addChild(submitBtn);

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

  auto tabContentLayout =
      CColumnLayoutBuilder::begin()
          ->gap(4)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  tabContentLayout->setMargin(4);
  scrollArea->addChild(tabContentLayout);
  tabMainLayout->addChild(scrollArea);

  if (m_searching) {
    auto searchingText =
        CTextBuilder::begin()
            ->text(std::string("⏳ Searching YouTube with yt-dlp..."))
            ->color([palette] {
              return palette ? palette->m_colors.accent
                             : CHyprColor(0.2, 0.8, 0.4, 1.0);
            })
            ->fontFamily(std::string(fontFamily))
            ->fontSize(CFontSize(CFontSize::HT_FONT_H3))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
            ->commence();
    tabContentLayout->addChild(searchingText);
  } else {
    if (m_results.empty()) {
      auto emptyText =
          CTextBuilder::begin()
              ->text(std::string("No search results. Enter a title or URL above, then click Search."))
              ->color([palette] {
                return palette ? palette->m_colors.text
                               : CHyprColor(0.7, 0.7, 0.7, 1.0);
              })
              ->fontFamily(std::string(fontFamily))
              ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
              ->size(CDynamicSize(CDynamicSize::HT_SIZE_AUTO,
                                  CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
              ->commence();
      tabContentLayout->addChild(emptyText);
    } else {
      int idx = 1;
      for (const auto &res : m_results) {
        std::string itemTitle = res.title;
        std::string itemUploader = res.uploader;
        std::string itemDuration = res.duration;
        std::string itemUrl = res.url;

        auto triggerPlayStream = [this, itemUrl, itemTitle, itemUploader]() {
          if (m_ctx.showNotification)
            m_ctx.showNotification("⏳ Resolving stream link with yt-dlp...");

          std::thread([this, itemUrl, itemTitle, itemUploader]() {
            std::string realUrl = extractDirectStreamUrl(itemUrl);
            if (realUrl.empty())
              realUrl = itemUrl;

            if (m_ctx.ytDlpService) {
              m_ctx.ytDlpService->setUrlTitle(realUrl, itemTitle, itemUploader);
              m_ctx.ytDlpService->setUrlTitle(itemUrl, itemTitle, itemUploader);
            }

            if (m_ctx.backend) {
              m_ctx.backend->addTimer(
                  std::chrono::milliseconds(1),
                  [this, realUrl](CAtomicSharedPointer<CTimer>, void *) {
                    if (m_ctx.playSongFromUri)
                      m_ctx.playSongFromUri(realUrl);
                  },
                  nullptr);
            }
          }).detach();
        };

        // ── Subtitle: "Uploader • Duration" ───────────────────────────────────
        std::string subtitle;
        if (!itemUploader.empty()) subtitle = itemUploader;
        if (!itemDuration.empty()) {
          subtitle += subtitle.empty() ? itemDuration : " \u2022 " + itemDuration;
        }

        // ── Build card via reusable SongCard component ────────────────────────
        auto card = std::make_shared<UI::Components::SongCard>(
            UI::Components::SongCardConfig{
                .palette    = palette,
                .fontFamily = fontFamily,
                .rounding   = palette ? palette->m_vars.smallRounding : 6,
                .cardHeight = 70.0f,
                .title      = std::to_string(idx++) + ". " + itemTitle,
                .subtitle   = subtitle,
                .imagePath  = Utils::getDefaultArtworkPath(),
                .isActive   = false,
                .onCardBodyClick = triggerPlayStream,
                .onActionClick   = [this, itemUrl, itemTitle, itemUploader,
                                    triggerPlayStream] {
                  Dialogs::showActionMenuDialog({
                      .options  = {"▶ Play Stream", "➕ Add Stream to Queue",
                                   "📁 Add Stream to Playlist", "🔗 Copy Link",
                                   "📥 Download to Database"},
                      .onSelect =
                          [this, itemUrl, itemTitle, itemUploader,
                           triggerPlayStream](size_t idx,
                                             const std::string &) {
                            if (idx == 0) {
                              triggerPlayStream();
                            } else if (idx == 1) {
                              if (m_ctx.showNotification)
                                m_ctx.showNotification(
                                    "⏳ Resolving stream link with yt-dlp...");
                              std::thread([this, itemUrl, itemTitle,
                                           itemUploader]() {
                                std::string realUrl =
                                    extractDirectStreamUrl(itemUrl);
                                if (realUrl.empty())
                                  realUrl = itemUrl;
                                if (m_ctx.ytDlpService) {
                                  m_ctx.ytDlpService->setUrlTitle(
                                      realUrl, itemTitle, itemUploader);
                                  m_ctx.ytDlpService->setUrlTitle(
                                      itemUrl, itemTitle, itemUploader);
                                }
                                if (m_ctx.backend)
                                  m_ctx.backend->addTimer(
                                      std::chrono::milliseconds(1),
                                      [this,
                                       realUrl](CAtomicSharedPointer<CTimer>,
                                                void *) {
                                        if (m_ctx.addSongToQueue)
                                          m_ctx.addSongToQueue(realUrl);
                                      },
                                      nullptr);
                              }).detach();
                            } else if (idx == 2) {
                              if (m_ctx.ytDlpService)
                                m_ctx.ytDlpService->setUrlTitle(
                                    itemUrl, itemTitle, itemUploader);
                              if (m_ctx.backend)
                                m_ctx.backend->addTimer(
                                    std::chrono::milliseconds(1),
                                    [this,
                                     itemUrl](CAtomicSharedPointer<CTimer>,
                                              void *) {
                                      if (m_ctx.showPlaylistSelectionDialog)
                                        m_ctx.showPlaylistSelectionDialog(
                                            itemUrl);
                                    },
                                    nullptr);
                            } else if (idx == 3) {
                              if (m_ctx.showNotification)
                                m_ctx.showNotification(
                                    "⏳ Extracting stream link with yt-dlp...");
                              std::thread([this, itemUrl, itemTitle,
                                           itemUploader]() {
                                std::string directUrl =
                                    extractDirectStreamUrl(itemUrl);
                                if (directUrl.empty())
                                  directUrl = itemUrl;
                                if (m_ctx.ytDlpService) {
                                  m_ctx.ytDlpService->setUrlTitle(
                                      directUrl, itemTitle, itemUploader);
                                  m_ctx.ytDlpService->setUrlTitle(
                                      itemUrl, itemTitle, itemUploader);
                                }
                                bool ok = Utils::copyToClipboard(directUrl);
                                if (m_ctx.backend)
                                  m_ctx.backend->addTimer(
                                      std::chrono::milliseconds(1),
                                      [this,
                                       ok](CAtomicSharedPointer<CTimer>,
                                           void *) {
                                        if (m_ctx.showNotification)
                                          m_ctx.showNotification(
                                              ok ? "📋 Copied direct stream "
                                                   "link to clipboard!"
                                                 : "❌ Failed to copy stream "
                                                   "link");
                                      },
                                      nullptr);
                              }).detach();
                            } else if (idx == 4) {
                              std::string musicDir =
                                  m_ctx.getMusicDirectory
                                      ? m_ctx.getMusicDirectory()
                                      : "";
                              Dialogs::showDownloadProgressDialog({
                                  .title    = itemTitle,
                                  .url      = itemUrl,
                                  .destDir  = musicDir,
                                  .parentWindow = m_ctx.window,
                                  .backend      = m_ctx.backend,
                                  .palette      = m_ctx.palette,
                                  .fontFamily   = m_ctx.fontFamily,
                                  .showNotification = m_ctx.showNotification,
                                  .runMpdCommand    = m_ctx.runMpdCommand});
                            }
                          },
                      .parentWindow = m_ctx.window,
                      .backend      = m_ctx.backend,
                      .palette      = m_ctx.palette,
                      .fontFamily   = m_ctx.fontFamily});
                }});
        tabContentLayout->addChild(card->build());
      }
    }
  }

  m_tabContentWrapper->forceReposition();
}

} // namespace UI::Views

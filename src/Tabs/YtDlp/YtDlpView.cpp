#include "YtDlpView.hpp"
#include "Dialogs/ActionMenuDialog.hpp"
#include "Dialogs/DialogCoordinator.hpp"
#include "Dialogs/DownloadProgressDialog.hpp"
#include "Tabs/Shared/SongCard.hpp"
#include "Utils/IconProvider.hpp"
#include "Utils/ArtworkUtils.hpp"
#include "Utils/ClipboardUtils.hpp"
#include "Utils/StreamUtils.hpp"
#include <algorithm>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <thread>

namespace UI::Views {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

namespace {

// --- Async Stream Resolution Helpers ---

void asyncPlayStream(const YtDlpViewContext &ctx, const std::string &itemUrl,
                     const std::string &itemTitle,
                     const std::string &itemUploader) {
  if (ctx.showNotification)
    ctx.showNotification(
        Components::IconProvider::getIcon(Components::IconType::LOADING) +
        " Resolving stream link with yt-dlp...");

  std::thread([ctx, itemUrl, itemTitle, itemUploader]() {
    std::string realUrl = extractDirectStreamUrl(itemUrl);
    if (realUrl.empty())
      realUrl = itemUrl;

    if (ctx.ytDlpService) {
      ctx.ytDlpService->setUrlTitle(realUrl, itemTitle, itemUploader);
      ctx.ytDlpService->setUrlTitle(itemUrl, itemTitle, itemUploader);
    }

    if (ctx.backend) {
      ctx.backend->addTimer(
          std::chrono::milliseconds(1),
          [ctx, realUrl](CAtomicSharedPointer<CTimer>, void *) {
            if (ctx.playSongFromUri)
              ctx.playSongFromUri(realUrl);
          },
          nullptr);
    }
  }).detach();
}

void asyncQueueStream(const YtDlpViewContext &ctx, const std::string &itemUrl,
                      const std::string &itemTitle,
                      const std::string &itemUploader) {
  if (ctx.showNotification)
    ctx.showNotification(
        Components::IconProvider::getIcon(Components::IconType::LOADING) +
        " Resolving stream link with yt-dlp...");

  std::thread([ctx, itemUrl, itemTitle, itemUploader]() {
    std::string realUrl = extractDirectStreamUrl(itemUrl);
    if (realUrl.empty())
      realUrl = itemUrl;

    if (ctx.ytDlpService) {
      ctx.ytDlpService->setUrlTitle(realUrl, itemTitle, itemUploader);
      ctx.ytDlpService->setUrlTitle(itemUrl, itemTitle, itemUploader);
    }

    if (ctx.backend) {
      ctx.backend->addTimer(
          std::chrono::milliseconds(1),
          [ctx, realUrl](CAtomicSharedPointer<CTimer>, void *) {
            if (ctx.addSongToQueue)
              ctx.addSongToQueue(realUrl);
          },
          nullptr);
    }
  }).detach();
}

void asyncCopyStreamUrl(const YtDlpViewContext &ctx, const std::string &itemUrl,
                        const std::string &itemTitle,
                        const std::string &itemUploader) {
  if (ctx.showNotification)
    ctx.showNotification("⏳ Extracting stream link with yt-dlp...");

  std::thread([ctx, itemUrl, itemTitle, itemUploader]() {
    std::string directUrl = extractDirectStreamUrl(itemUrl);
    if (directUrl.empty())
      directUrl = itemUrl;

    if (ctx.ytDlpService) {
      ctx.ytDlpService->setUrlTitle(directUrl, itemTitle, itemUploader);
      ctx.ytDlpService->setUrlTitle(itemUrl, itemTitle, itemUploader);
    }

    bool ok = Utils::copyToClipboard(directUrl);
    if (ctx.backend) {
      ctx.backend->addTimer(
          std::chrono::milliseconds(1),
          [ctx, ok](CAtomicSharedPointer<CTimer>, void *) {
            if (ctx.showNotification)
              ctx.showNotification(
                  ok ? Components::IconProvider::getIcon(
                           Components::IconType::COPY) +
                           " Copied direct stream link to clipboard!"
                     : Components::IconProvider::getIcon(
                           Components::IconType::CROSS) +
                           " Failed to copy stream link");
          },
          nullptr);
    }
  }).detach();
}

// --- Helper: Render SongCard Component for YT-DLP Search Result ---

std::shared_ptr<UI::Components::SongCard> renderYtDlpCard(
    const YtDlpViewContext &ctx, const Services::YtDlpResult &res, int index,
    int rounding) {
  std::string itemTitle = res.title;
  std::string itemUploader = res.uploader;
  std::string itemDuration = res.duration;
  std::string itemUrl = res.url;

  std::string subtitle;
  if (!itemUploader.empty())
    subtitle = itemUploader;
  if (!itemDuration.empty()) {
    subtitle += subtitle.empty() ? itemDuration : " \u2022 " + itemDuration;
  }

  auto triggerPlay = [ctx, itemUrl, itemTitle, itemUploader]() {
    asyncPlayStream(ctx, itemUrl, itemTitle, itemUploader);
  };

  return std::make_shared<UI::Components::SongCard>(
      UI::Components::SongCardConfig{
          .palette = ctx.palette,
          .fontFamily = ctx.fontFamily,
          .rounding = rounding,
          .cardHeight = 70.0f,
          .title = std::to_string(index) + ". " + itemTitle,
          .subtitle = subtitle,
          .imagePath = Utils::getDefaultArtworkPath(),
          .isActive = false,
          .onCardBodyClick = triggerPlay,
          .onActionClick =
              [ctx, itemUrl, itemTitle, itemUploader, triggerPlay] {
                Dialogs::ActionMenuContext menuCtx{
                    .options = {Components::IconProvider::getIcon(
                                    Components::IconType::PLAY) +
                                    " Play Stream",
                                Components::IconProvider::getIcon(
                                    Components::IconType::ADD_TO_QUEUE) +
                                    " Add Stream to Queue",
                                Components::IconProvider::getIcon(
                                    Components::IconType::ADD_TO_PLAYLIST) +
                                    " Add Stream to Playlist",
                                Components::IconProvider::getIcon(
                                    Components::IconType::COPY) +
                                    " Copy Link",
                                Components::IconProvider::getIcon(
                                    Components::IconType::DOWNLOAD) +
                                    " Download to Database"},
                    .onSelect =
                        [ctx, itemUrl, itemTitle, itemUploader,
                         triggerPlay](size_t idx, const std::string &) {
                          if (idx == 0) {
                            triggerPlay();
                          } else if (idx == 1) {
                            asyncQueueStream(ctx, itemUrl, itemTitle,
                                             itemUploader);
                          } else if (idx == 2) {
                            if (ctx.ytDlpService)
                              ctx.ytDlpService->setUrlTitle(
                                  itemUrl, itemTitle, itemUploader);
                            if (ctx.dialogCoordinator) {
                              ctx.dialogCoordinator->showPlaylistSelectionDialog(itemUrl);
                            } else if (ctx.backend) {
                              ctx.backend->addTimer(
                                  std::chrono::milliseconds(1),
                                  [ctx, itemUrl](CAtomicSharedPointer<CTimer>,
                                                 void *) {
                                    if (ctx.showPlaylistSelectionDialog)
                                      ctx.showPlaylistSelectionDialog(itemUrl);
                                  },
                                  nullptr);
                            }
                          } else if (idx == 3) {
                            asyncCopyStreamUrl(ctx, itemUrl, itemTitle,
                                               itemUploader);
                          } else if (idx == 4) {
                            std::string musicDir =
                                ctx.getMusicDirectory ? ctx.getMusicDirectory()
                                                      : "";
                            if (ctx.dialogCoordinator) {
                              ctx.dialogCoordinator->showDownloadProgressDialog(
                                  itemTitle, itemUrl, musicDir);
                            } else {
                              Dialogs::showDownloadProgressDialog({
                                  .title = itemTitle,
                                  .url = itemUrl,
                                  .destDir = musicDir,
                                  .parentWindow = ctx.window,
                                  .backend = ctx.backend,
                                  .palette = ctx.palette,
                                  .fontFamily = ctx.fontFamily,
                                  .showNotification = ctx.showNotification,
                                  .runMpdCommand = ctx.runMpdCommand});
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

} // namespace

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
        [this](const std::vector<Services::YtDlpResult> &results, bool /*isPlaylist*/,
               const std::string &/*plTitle*/, const std::string &/*plId*/) {
          if (m_ctx.backend) {
            m_ctx.backend->addTimer(
                std::chrono::milliseconds(1),
                [this, results](CAtomicSharedPointer<CTimer>, void *) {
                  m_searching = false;
                  m_results = results;
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

void YtDlpView::rebuildUI(CSharedPointer<CRectangleElement> wrapper,
                          struct mpd_connection *conn) {
  (void)conn;
  m_tabContentWrapper = wrapper;
  if (!m_tabContentWrapper)
    return;

  m_tabContentWrapper->clearChildren();

  auto palette = m_ctx.palette;
  std::string fontFamily = m_ctx.fontFamily;
  int rounding = palette ? palette->m_vars.smallRounding : 6;

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

  auto titleInput =
      CTextboxBuilder::begin()
          ->placeholder("Search YouTube")
          ->defaultText(std::string(m_searchTitle))
          ->onTextEdited([this](CSharedPointer<CTextboxElement>, const std::string &text) {
            m_searchTitle = text;
          })
          ->multiline(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 40.0F}))
          ->commence();
  titleInput->setGrow(true);
  topSearchRow->addChild(titleInput);

  auto pasteBtn = CButtonBuilder::begin()
                      ->label(Components::IconProvider::getIcon(Components::IconType::PASTE))
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
                                          {40.0F, 40.0F}))
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
          ->multiline(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {45.0F, 40.0F}))
          ->commence();
  countInput->setGrow(false);
  topSearchRow->addChild(countInput);

  auto submitBtn = CButtonBuilder::begin()
                       ->label("Go")
                       ->alignText(HT_FONT_ALIGN_CENTER)
                       ->fontFamily(std::string(fontFamily))
                       ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
                       ->onMainClick([this](CSharedPointer<CButtonElement>) { triggerSearch(); })
                       ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                           CDynamicSize::HT_SIZE_ABSOLUTE, {60.0F, 40.0F}))
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
            ->text(Components::IconProvider::getIcon(Components::IconType::LOADING) +
                   " Searching YouTube with yt-dlp...")
            ->color([palette] {
              return palette ? palette->m_colors.text
                             : CHyprColor(1.0, 1.0, 1.0, 1.0);
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
        auto card = renderYtDlpCard(m_ctx, res, idx++, rounding);
        tabContentLayout->addChild(card->build());
      }
    }
  }

  m_tabContentWrapper->forceReposition();
}

} // namespace UI::Views

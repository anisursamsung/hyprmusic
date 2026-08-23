#include "DownloadProgressDialog.hpp"
#include "BaseDialog.hpp"
#include "Utils/IconProvider.hpp"
#include "Utils/UIFactory.hpp"
#include "Utils/StreamUtils.hpp"
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <sys/wait.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <regex>
#include <thread>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

CSharedPointer<IWindow> showDownloadProgressDialog(const DownloadProgressContext &ctx) {
  if (!ctx.parentWindow || ctx.url.empty())
    return nullptr;

  auto palette = ctx.palette;
  std::string fontFamily = ctx.fontFamily;
  auto windowSize = ctx.parentWindow->pixelSize();
  double dialogW = std::clamp(460.0, 300.0, windowSize.x - 20.0);
  double dialogH = 220.0;

  auto res = BaseDialog::createCenteredModal(
      ctx.parentWindow,
      Hyprutils::Math::Vector2D(dialogW, dialogH),
      palette, 10.0f, 10, 12);

  if (!res.window)
    return nullptr;

  if (ctx.onWindowCreated) {
    ctx.onWindowCreated(res.window);
  }

  auto dialogWindow = res.window;

  auto headerText = BaseDialog::createHeader(
      "Downloading Track",
      Components::IconProvider::getIcon(Components::IconType::DOWNLOAD),
      palette, fontFamily);
  res.contentLayout->addChild(headerText);

  std::string trackTitleStr = ctx.title.empty() ? ctx.url : ctx.title;
  auto trackLabel =
      CTextBuilder::begin()
          ->text(std::string(trackTitleStr))
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(0.9, 0.9, 0.9, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->align(HT_FONT_ALIGN_LEFT)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  res.contentLayout->addChild(trackLabel);

  auto statusText =
      CTextBuilder::begin()
          ->text(Components::IconProvider::getIcon(Components::IconType::LOADING) + " Initializing download...")
          ->color([palette] {
            return palette ? palette->m_colors.text
                           : CHyprColor(1.0, 1.0, 1.0, 1.0);
          })
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->align(HT_FONT_ALIGN_LEFT)
          ->noEllipsize(false)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();
  res.contentLayout->addChild(statusText);

  auto progressBar =
      CSliderBuilder::begin()
          ->min(0.0f)
          ->max(1.0f)
          ->val(0.0f)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 14.0F}))
          ->commence();
  res.contentLayout->addChild(progressBar);

  auto isFinished = std::make_shared<std::atomic<bool>>(false);
  auto isCancelled = std::make_shared<std::atomic<bool>>(false);

  auto closeWindowLambda = [dialogWindow] {
    if (dialogWindow)
      dialogWindow->close();
  };

  auto actionBtnRow =
      CRowLayoutBuilder::begin()
          ->gap(0)
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                              CDynamicSize::HT_SIZE_AUTO, {1.0F, 1.0F}))
          ->commence();

  auto actionBtn =
      CButtonBuilder::begin()
          ->label("Cancel")
          ->alignText(HT_FONT_ALIGN_CENTER)
          ->fontFamily(std::string(fontFamily))
          ->fontSize(CFontSize(CFontSize::HT_FONT_TEXT))
          ->onMainClick([closeWindowLambda, isFinished, isCancelled](CSharedPointer<CButtonElement>) {
            if (!isFinished->load()) {
              isCancelled->store(true);
            }
            closeWindowLambda();
          })
          ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                              CDynamicSize::HT_SIZE_ABSOLUTE, {90.0F, 32.0F}))
          ->commence();
  actionBtn->setGrow(false);
  actionBtnRow->addChild(actionBtn);
  res.contentLayout->addChild(actionBtnRow);

  dialogWindow->open();

  // Launch background thread to run yt-dlp and parse progress output line-by-line
  std::thread([ctx, dialogWindow, statusText, progressBar, actionBtn, isFinished, isCancelled]() {
    std::string destDir = expandTilde(ctx.destDir);
    if (destDir.empty()) {
      destDir = getUserHomeDir() + "/Music";
    }

    try {
      std::filesystem::create_directories(destDir);
    } catch (...) {
    }

    if (ctx.backend && ctx.showNotification) {
      ctx.backend->addTimer(
          std::chrono::milliseconds(1),
          [ctx](CAtomicSharedPointer<CTimer>, void *) {
            if (ctx.showNotification) {
              std::string t = ctx.title.empty() ? "track" : "'" + ctx.title + "'";
              ctx.showNotification("⏳ Download started: " + t);
            }
          },
          nullptr);
    }

    std::string escapedUrl = escapeShellArg(ctx.url);
    std::string escapedDir = escapeShellArg(destDir);
    std::string outputTemplate = escapedDir + "/%(title)s [%(id)s].%(ext)s";

    // Use --newline to get discrete log lines and --restrict-filenames for safe cross-platform output files
    std::string cmd = "yt-dlp --newline --no-playlist -x --audio-format mp3 --audio-quality 0 --embed-thumbnail --embed-metadata --restrict-filenames -o \"" +
                      outputTemplate + "\" \"" + escapedUrl + "\" 2>&1";

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
      isFinished->store(true);
      if (ctx.backend) {
        ctx.backend->addTimer(
            std::chrono::milliseconds(1),
            [statusText, actionBtn](CAtomicSharedPointer<CTimer>, void *) {
              statusText->rebuild()->text(Components::IconProvider::getIcon(Components::IconType::CROSS) + " Failed to launch yt-dlp process")->commence();
              actionBtn->rebuild()->label(std::string("Close"))->commence();
            },
            nullptr);
      }
      return;
    }

    char buffer[4096];
    std::regex pctRegex(R"((\d+(?:\.\d+)?)%)");

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      if (isCancelled->load()) {
        break;
      }

      std::string line(buffer);

      if (line.find("[download]") != std::string::npos) {
        std::smatch match;
        float pctVal = -1.0f;
        if (std::regex_search(line, match, pctRegex) && match.size() > 1) {
          try {
            pctVal = std::stof(match[1].str()) / 100.0f;
          } catch (...) {
          }
        }

        // Clean trailing whitespace and newlines for clean status message
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')) {
          line.pop_back();
        }

        if (ctx.backend) {
          ctx.backend->addTimer(
              std::chrono::milliseconds(1),
              [statusText, progressBar, line, pctVal](CAtomicSharedPointer<CTimer>, void *) {
                statusText->rebuild()->text(std::string(line))->commence();
                if (pctVal >= 0.0f) {
                  progressBar->rebuild()->val(pctVal)->commence();
                }
              },
              nullptr);
        }
      } else if (line.find("[ExtractAudio]") != std::string::npos || line.find("[ffmpeg]") != std::string::npos) {
        if (ctx.backend) {
          ctx.backend->addTimer(
              std::chrono::milliseconds(1),
              [statusText, progressBar](CAtomicSharedPointer<CTimer>, void *) {
                statusText->rebuild()->text(Components::IconProvider::getIcon(Components::IconType::LOADING) + " Converting audio to MP3...")->commence();
                progressBar->rebuild()->val(0.92f)->commence();
              },
              nullptr);
        }
      } else if (line.find("[EmbedThumbnail]") != std::string::npos || line.find("[Metadata]") != std::string::npos || line.find("[ThumbnailsConvertor]") != std::string::npos) {
        if (ctx.backend) {
          ctx.backend->addTimer(
              std::chrono::milliseconds(1),
              [statusText, progressBar](CAtomicSharedPointer<CTimer>, void *) {
                statusText->rebuild()->text(Components::IconProvider::getIcon(Components::IconType::MUSIC_NOTE) + " Embedding thumbnail & metadata...")->commence();
                progressBar->rebuild()->val(0.98f)->commence();
              },
              nullptr);
        }
      }
    }

    int status = pclose(pipe);
    bool isSuccess = (status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    isFinished->store(true);

    if (isCancelled->load())
      return;

    if (ctx.backend) {
      ctx.backend->addTimer(
          std::chrono::milliseconds(1),
          [ctx, statusText, progressBar, actionBtn, isSuccess](CAtomicSharedPointer<CTimer>, void *) {
            std::string displayTitle = ctx.title.empty() ? "track" : "'" + ctx.title + "'";
            if (isSuccess) {
              progressBar->rebuild()->val(1.0f)->commence();
              statusText->rebuild()->text(Components::IconProvider::getIcon(Components::IconType::CHECK) + " Download completed!")->commence();
              actionBtn->rebuild()->label(std::string("Done"))->commence();

              if (ctx.showNotification) {
                ctx.showNotification(Components::IconProvider::getIcon(Components::IconType::CHECK) + " Download completed: " + displayTitle);
              }
              if (ctx.runMpdCommand) {
                ctx.runMpdCommand([](struct mpd_connection *conn) {
                  if (conn) {
                    mpd_run_update(conn, nullptr);
                  }
                });
              }
            } else {
              statusText->rebuild()->text(Components::IconProvider::getIcon(Components::IconType::CROSS) + " Download failed. Check yt-dlp & formats.")->commence();
              actionBtn->rebuild()->label(std::string("Close"))->commence();
              if (ctx.showNotification) {
                ctx.showNotification(Components::IconProvider::getIcon(Components::IconType::CROSS) + " Download failed: " + displayTitle);
              }
            }
          },
          nullptr);
    }
  }).detach();

  return dialogWindow;
}

} // namespace UI::Dialogs

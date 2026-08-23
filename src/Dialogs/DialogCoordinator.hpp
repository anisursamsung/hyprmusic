#pragma once

#include "Tabs/YtDlp/YtDlpService.hpp"
#include "ActionMenuDialog.hpp"
#include "AddItemDialog.hpp"
#include "ConfirmDialog.hpp"
#include "DownloadProgressDialog.hpp"
#include "PlaylistSelectionDialog.hpp"
#include "PromptDialog.hpp"
#include <functional>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <memory>
#include <mpd/client.h>
#include <string>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct DialogCoordinatorContext {
  CSharedPointer<IWindow> parentWindow;
  CSharedPointer<IBackend> backend;
  CSharedPointer<CPalette> palette;
  std::string fontFamily;

  Services::YtDlpService *ytDlpService = nullptr;

  std::function<void(const std::function<void(struct mpd_connection *)> &)>
      runMpdCommand;
  std::function<void(const std::string &msg)> showNotification;
  std::function<void()> updateStatus;
};

class DialogCoordinator {
public:
  explicit DialogCoordinator(const DialogCoordinatorContext &ctx);

  // Close any currently active modal or popup window
  void closeActiveDialog();

  // Generic Dialog Launchers
  void showPrompt(PromptDialogContext ctx);
  void showConfirm(ConfirmDialogContext ctx);
  void showActionMenu(ActionMenuContext ctx);

  // Semantic App Dialogs
  void showRenameDialog(const std::string &oldName,
                        std::function<void(const std::string &, const std::string &)> onRenamed = nullptr);
  void showCreatePlaylistDialog(std::function<void(const std::string &)> onCreated = nullptr);
  void showDeletePlaylistDialog(const std::string &plName,
                                std::function<void()> onDeleted = nullptr);
  void showClearQueueDialog(std::function<void()> onCleared = nullptr);
  void showPlaylistSelectionDialog(const std::string &songUri,
                                  const std::string &currentPlaylist = "",
                                  int moveFromSongPos = -1,
                                  std::function<void()> onUpdated = nullptr);
  void showQueueAddItemDialog();
  void showPlaylistAddItemDialog(const std::string &playlistName,
                                 std::function<void()> onUpdated = nullptr);
  void showDownloadProgressDialog(const std::string &title,
                                  const std::string &url,
                                  const std::string &destDir = "");

  // Safe UI dispatch helper to post callbacks to the main loop
  void dispatchToMainThread(std::function<void()> callback, int delayMs = 1);

  const DialogCoordinatorContext &getContext() const { return m_ctx; }

private:
  DialogCoordinatorContext m_ctx;
  CSharedPointer<IWindow> m_activeDialogWindow = nullptr;
};

} // namespace UI::Dialogs

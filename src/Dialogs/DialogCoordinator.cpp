#include "DialogCoordinator.hpp"
#include "MPD/MPDManager.hpp"
#include "Utils/IconProvider.hpp"

namespace UI::Dialogs {

DialogCoordinator::DialogCoordinator(const DialogCoordinatorContext &ctx)
    : m_ctx(ctx) {}

void DialogCoordinator::closeActiveDialog() {
  if (m_activeDialogWindow) {
    m_activeDialogWindow->close();
    m_activeDialogWindow = nullptr;
  }
}

void DialogCoordinator::dispatchToMainThread(std::function<void()> callback, int delayMs) {
  if (!callback)
    return;
  if (m_ctx.backend) {
    m_ctx.backend->addTimer(
        std::chrono::milliseconds(delayMs),
        [callback](CAtomicSharedPointer<CTimer>, void *) { callback(); },
        nullptr);
  } else {
    callback();
  }
}

void DialogCoordinator::showPrompt(PromptDialogContext ctx) {
  closeActiveDialog();
  if (!ctx.parentWindow)
    ctx.parentWindow = m_ctx.parentWindow;
  if (!ctx.backend)
    ctx.backend = m_ctx.backend;
  if (!ctx.palette)
    ctx.palette = m_ctx.palette;
  if (ctx.fontFamily.empty())
    ctx.fontFamily = m_ctx.fontFamily;

  ctx.onWindowCreated = [this](CSharedPointer<IWindow> w) {
    m_activeDialogWindow = w;
  };
  showPromptDialog(ctx);
}

void DialogCoordinator::showConfirm(ConfirmDialogContext ctx) {
  closeActiveDialog();
  if (!ctx.parentWindow)
    ctx.parentWindow = m_ctx.parentWindow;
  if (!ctx.backend)
    ctx.backend = m_ctx.backend;
  if (!ctx.palette)
    ctx.palette = m_ctx.palette;
  if (ctx.fontFamily.empty())
    ctx.fontFamily = m_ctx.fontFamily;

  ctx.onWindowCreated = [this](CSharedPointer<IWindow> w) {
    m_activeDialogWindow = w;
  };
  showConfirmDialog(ctx);
}

void DialogCoordinator::showActionMenu(ActionMenuContext ctx) {
  closeActiveDialog();
  if (!ctx.parentWindow)
    ctx.parentWindow = m_ctx.parentWindow;
  if (!ctx.backend)
    ctx.backend = m_ctx.backend;
  if (!ctx.palette)
    ctx.palette = m_ctx.palette;
  if (ctx.fontFamily.empty())
    ctx.fontFamily = m_ctx.fontFamily;

  ctx.onWindowCreated = [this](CSharedPointer<IWindow> w) {
    m_activeDialogWindow = w;
  };
  showActionMenuDialog(ctx);
}

void DialogCoordinator::showRenameDialog(
    const std::string &oldName,
    std::function<void(const std::string &, const std::string &)> onRenamed) {
  PromptDialogContext ctx{
      .title = "Rename Playlist",
      .icon = Components::IconProvider::getIcon(Components::IconType::EDIT),
      .placeholder = "Playlist name...",
      .initialText = oldName,
      .confirmLabel = "Ok",
      .confirmIsAccent = true,
      .onConfirm = [this, oldName, onRenamed](const std::string &newName) {
        if (!newName.empty() && newName != oldName) {
          m_ctx.runMpdCommand([oldName, newName](struct mpd_connection *conn) {
            mpd_run_rename(conn, oldName.c_str(), newName.c_str());
          });
          if (onRenamed) {
            onRenamed(oldName, newName);
          }
        }
      }};
  showPrompt(ctx);
}

void DialogCoordinator::showCreatePlaylistDialog(
    std::function<void(const std::string &)> onCreated) {
  PromptDialogContext ctx{
      .title = "Create New Playlist",
      .icon = Components::IconProvider::getIcon(Components::IconType::ADD),
      .placeholder = "Playlist name...",
      .initialText = "",
      .confirmLabel = "Ok",
      .confirmIsAccent = true,
      .onConfirm = [this, onCreated](const std::string &plName) {
        if (!plName.empty()) {
          m_ctx.runMpdCommand([plName](struct mpd_connection *conn) {
            mpd_run_save(conn, plName.c_str());
            mpd_run_playlist_clear(conn, plName.c_str());
          });
          if (onCreated) {
            onCreated(plName);
          }
        }
      }};
  showPrompt(ctx);
}

void DialogCoordinator::showDeletePlaylistDialog(
    const std::string &plName, std::function<void()> onDeleted) {
  ConfirmDialogContext ctx{
      .title = "Delete Playlist",
      .icon = Components::IconProvider::getIcon(Components::IconType::DELETE),
      .message = "Are you sure you want to delete playlist '" + plName + "'?",
      .cancelLabel = "Cancel",
      .confirmLabel = "Delete",
      .isDestructive = true,
      .onConfirm = [this, plName, onDeleted] {
        m_ctx.runMpdCommand([plName](struct mpd_connection *conn) {
          mpd_run_playlist_clear(conn, plName.c_str());
          mpd_run_rm(conn, plName.c_str());
        });
        if (m_ctx.showNotification)
          m_ctx.showNotification("Deleted " + plName);
        if (onDeleted)
          onDeleted();
        if (m_ctx.updateStatus)
          m_ctx.updateStatus();
      }};
  showConfirm(ctx);
}

void DialogCoordinator::showClearQueueDialog(std::function<void()> onCleared) {
  ConfirmDialogContext ctx{
      .title = "Clear Queue",
      .icon = Components::IconProvider::getIcon(Components::IconType::CLEAR_ALL),
      .message = "Are you sure you want to clear the playback queue?",
      .cancelLabel = "Cancel",
      .confirmLabel = "Clear",
      .isDestructive = true,
      .onConfirm = [this, onCleared] {
        m_ctx.runMpdCommand([onCleared](struct mpd_connection *conn) {
          if (!conn)
            return;
          mpd_run_clear(conn);
          if (onCleared)
            onCleared();
        });
        if (m_ctx.showNotification)
          m_ctx.showNotification("Queue cleared");
        if (m_ctx.updateStatus)
          m_ctx.updateStatus();
      }};
  showConfirm(ctx);
}

void DialogCoordinator::showPlaylistSelectionDialog(
    const std::string &songUri, const std::string &currentPlaylist,
    int moveFromSongPos, std::function<void()> onUpdated) {
  closeActiveDialog();
  PlaylistSelectionContext ctx{
      .songUri = songUri,
      .moveFromSongPos = moveFromSongPos,
      .currentSelectedPlaylist = currentPlaylist,
      .parentWindow = m_ctx.parentWindow,
      .backend = m_ctx.backend,
      .palette = m_ctx.palette,
      .fontFamily = m_ctx.fontFamily,
      .ytDlpService = m_ctx.ytDlpService,
      .runMpdCommand = m_ctx.runMpdCommand,
      .showNotification = m_ctx.showNotification,
      .onPlaylistUpdated = onUpdated,
      .onWindowCreated = [this](CSharedPointer<IWindow> w) {
        m_activeDialogWindow = w;
      }};
  UI::Dialogs::showPlaylistSelectionDialog(ctx);
}

void DialogCoordinator::showQueueAddItemDialog() {
  closeActiveDialog();
  AddItemDialogContext ctx;
  ctx.targetType = AddItemTargetType::QUEUE;
  ctx.window = m_ctx.parentWindow;
  ctx.backend = m_ctx.backend;
  ctx.palette = m_ctx.palette;
  ctx.fontFamily = m_ctx.fontFamily;
  ctx.runMpdCommand = m_ctx.runMpdCommand;
  ctx.showNotification = m_ctx.showNotification;
  ctx.addSongToQueue = [this](const std::string &uri) {
    Services::MPDManager::addSongToQueue(
        uri, [this](const std::string &m) { m_ctx.showNotification(m); },
        [this] {
          if (m_ctx.updateStatus)
            m_ctx.updateStatus();
        });
  };
  ctx.refreshCallback = [this] {
    if (m_ctx.updateStatus)
      m_ctx.updateStatus();
  };
  ctx.onWindowCreated = [this](CSharedPointer<IWindow> w) {
    m_activeDialogWindow = w;
  };
  UI::Dialogs::showAddItemDialog(ctx);
}

void DialogCoordinator::showPlaylistAddItemDialog(
    const std::string &playlistName, std::function<void()> onUpdated) {
  closeActiveDialog();
  AddItemDialogContext ctx;
  ctx.targetType = AddItemTargetType::PLAYLIST;
  ctx.targetPlaylistName = playlistName;
  ctx.window = m_ctx.parentWindow;
  ctx.backend = m_ctx.backend;
  ctx.palette = m_ctx.palette;
  ctx.fontFamily = m_ctx.fontFamily;
  ctx.runMpdCommand = m_ctx.runMpdCommand;
  ctx.showNotification = m_ctx.showNotification;
  ctx.addSongToQueue = [this](const std::string &uri) {
    Services::MPDManager::addSongToQueue(
        uri, [this](const std::string &m) { m_ctx.showNotification(m); },
        [this] {
          if (m_ctx.updateStatus)
            m_ctx.updateStatus();
        });
  };
  ctx.refreshCallback = onUpdated;
  ctx.onWindowCreated = [this](CSharedPointer<IWindow> w) {
    m_activeDialogWindow = w;
  };
  UI::Dialogs::showAddItemDialog(ctx);
}

void DialogCoordinator::showDownloadProgressDialog(
    const std::string &title, const std::string &url,
    const std::string &destDir) {
  closeActiveDialog();
  DownloadProgressContext ctx{
      .title = title,
      .url = url,
      .destDir = destDir,
      .parentWindow = m_ctx.parentWindow,
      .backend = m_ctx.backend,
      .palette = m_ctx.palette,
      .fontFamily = m_ctx.fontFamily,
      .showNotification = m_ctx.showNotification,
      .runMpdCommand = m_ctx.runMpdCommand,
      .onWindowCreated = [this](CSharedPointer<IWindow> w) {
        m_activeDialogWindow = w;
      }};
  UI::Dialogs::showDownloadProgressDialog(ctx);
}

} // namespace UI::Dialogs

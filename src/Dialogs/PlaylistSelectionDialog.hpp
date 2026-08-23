#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <string>
#include <vector>
#include "Tabs/YtDlp/YtDlpService.hpp"

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct PlaylistSelectionContext {
  std::string songUri = "";
  int moveFromSongPos = -1;
  std::string currentSelectedPlaylist = "";

  CSharedPointer<IWindow> parentWindow = nullptr;
  CSharedPointer<IBackend> backend = nullptr;
  CSharedPointer<CPalette> palette = nullptr;
  std::string fontFamily = "";

  Services::YtDlpService *ytDlpService = nullptr;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand = nullptr;
  std::function<void(const std::string &msg)> showNotification = nullptr;
  std::function<void()> onPlaylistUpdated = nullptr;
  std::function<void(CSharedPointer<IWindow>)> onWindowCreated = nullptr;
};

CSharedPointer<IWindow> showPlaylistSelectionDialog(const PlaylistSelectionContext &ctx);

} // namespace UI::Dialogs

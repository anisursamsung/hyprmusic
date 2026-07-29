#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <string>
#include <vector>
#include "../../Services/YtDlpService.hpp"

namespace UI::Dialogs {

struct PlaylistSelectionContext {
  std::string songUri = "";
  int moveFromSongPos = -1;
  std::string currentSelectedPlaylist = "";

  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> parentWindow = nullptr;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend = nullptr;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette = nullptr;
  std::string fontFamily = "";

  Services::YtDlpService *ytDlpService = nullptr;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand = nullptr;
  std::function<void(const std::string &msg)> showNotification = nullptr;
  std::function<void()> onPlaylistUpdated = nullptr;
};

void showPlaylistSelectionDialog(const PlaylistSelectionContext &ctx);

} // namespace UI::Dialogs

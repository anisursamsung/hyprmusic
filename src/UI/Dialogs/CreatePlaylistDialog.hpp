#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <string>

namespace UI::Dialogs {

struct CreatePlaylistContext {
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> parentWindow;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette;
  std::string fontFamily;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
  std::function<void(const std::string &msg)> showNotification;
  std::function<void(const std::string &plName)> onCreated;
};

void showCreatePlaylistDialog(const CreatePlaylistContext &ctx);

} // namespace UI::Dialogs

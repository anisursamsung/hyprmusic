#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <string>

namespace UI::Dialogs {

struct RenamePlaylistContext {
  std::string oldName;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> parentWindow;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette;
  std::string fontFamily;

  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand;
  std::function<void(const std::string &oldName, const std::string &newName)> onRenamed;
};

void showRenamePlaylistDialog(const RenamePlaylistContext &ctx);

} // namespace UI::Dialogs

#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <memory>
#include <string>

namespace UI::Dialogs {

enum class AddItemTargetType {
  QUEUE,
  PLAYLIST
};

struct AddItemDialogContext {
  AddItemTargetType targetType = AddItemTargetType::QUEUE;
  std::string targetPlaylistName;

  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette;
  std::string fontFamily;

  std::function<void(std::function<void(struct mpd_connection *)>)> runMpdCommand;
  std::function<void(const std::string &)> showNotification;
  std::function<void(const std::string &)> addSongToQueue;
  std::function<void()> refreshCallback;
};

void showAddItemDialog(const AddItemDialogContext &ctx);

} // namespace UI::Dialogs

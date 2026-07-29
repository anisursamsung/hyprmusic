#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <string>

namespace UI::Dialogs {

struct DownloadProgressContext {
  std::string title = "";
  std::string url = "";
  std::string destDir = "";

  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> parentWindow = nullptr;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend = nullptr;
  Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette = nullptr;
  std::string fontFamily = "";

  std::function<void(const std::string &msg)> showNotification = nullptr;
  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand = nullptr;
};

void showDownloadProgressDialog(const DownloadProgressContext &ctx);

} // namespace UI::Dialogs

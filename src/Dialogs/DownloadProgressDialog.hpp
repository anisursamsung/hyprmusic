#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <mpd/client.h>
#include <functional>
#include <string>

namespace UI::Dialogs {

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

struct DownloadProgressContext {
  std::string title = "";
  std::string url = "";
  std::string destDir = "";

  CSharedPointer<IWindow> parentWindow = nullptr;
  CSharedPointer<IBackend> backend = nullptr;
  CSharedPointer<CPalette> palette = nullptr;
  std::string fontFamily = "";

  std::function<void(const std::string &msg)> showNotification = nullptr;
  std::function<void(const std::function<void(struct mpd_connection *)> &)> runMpdCommand = nullptr;
  std::function<void(CSharedPointer<IWindow>)> onWindowCreated = nullptr;
};

CSharedPointer<IWindow> showDownloadProgressDialog(const DownloadProgressContext &ctx);

} // namespace UI::Dialogs

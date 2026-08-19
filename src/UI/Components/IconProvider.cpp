#include "IconProvider.hpp"
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

namespace UI::Components {

static bool g_fontRegistered = false;

void IconProvider::registerCustomFont() {
  if (g_fontRegistered)
    return;

  std::vector<std::filesystem::path> candidatePaths;

  // 1. Check relative to current working directory
  candidatePaths.push_back(std::filesystem::current_path() / "assets/fonts/hyprmusic.ttf");
  candidatePaths.push_back(std::filesystem::current_path() / "../assets/fonts/hyprmusic.ttf");

  // 2. Check relative to executable location via /proc/self/exe
  try {
    std::filesystem::path exePath = std::filesystem::read_symlink("/proc/self/exe");
    std::filesystem::path exeDir = exePath.parent_path();
    candidatePaths.push_back(exeDir / "assets/fonts/hyprmusic.ttf");
    candidatePaths.push_back(exeDir / "../assets/fonts/hyprmusic.ttf");
    candidatePaths.push_back(exeDir / "fonts/hyprmusic.ttf");
  } catch (...) {}

  // 3. Fallback system installation paths
  candidatePaths.push_back("/usr/share/hyprmusic/assets/fonts/hyprmusic.ttf");
  candidatePaths.push_back("/usr/local/share/hyprmusic/assets/fonts/hyprmusic.ttf");

  const char *homeEnv = std::getenv("HOME");
  if (homeEnv) {
    candidatePaths.push_back(std::filesystem::path(homeEnv) / ".local/share/fonts/hyprmusic.ttf");
  }

  FcConfig *config = FcConfigGetCurrent();
  if (!config) {
    config = FcInitLoadConfigAndFonts();
    FcConfigSetCurrent(config);
  }

  for (const auto &path : candidatePaths) {
    if (std::filesystem::exists(path)) {
      std::string absPathStr = std::filesystem::absolute(path).string();
      if (FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8 *>(absPathStr.c_str())) == FcTrue) {
        FcConfigBuildFonts(config);
        pango_cairo_font_map_set_default(NULL);
        g_fontRegistered = true;
        break;
      }
    }
  }
}

std::string IconProvider::getCustomFontFamily() {
  registerCustomFont();
  return "hyprmusic";
}

bool IconProvider::isCustomFontIcon(IconType /*type*/) {
  return true;
}

bool IconProvider::isCustomFontIcon(const std::string &/*iconStr*/) {
  return true;
}

std::string IconProvider::getIcon(IconType type) {
  switch (type) {
  // Transport Controls
  case IconType::PLAY:
    return "\uE90C";
  case IconType::PAUSE:
    return "\uE910";
  case IconType::PREV_TRACK:
    return "\uE91A";
  case IconType::NEXT_TRACK:
    return "\uE90D";
  case IconType::SHUFFLE:
    return "\uE914";

  // Volume
  case IconType::VOLUME_MUTED:
    return "\uE917";
  case IconType::VOLUME_LOW:
  case IconType::VOLUME_MEDIUM:
    return "\uE916";
  case IconType::VOLUME_HIGH:
    return "\uE918";

  // Navigation Tabs
  case IconType::NAV_QUEUE:
    return "\uE911";
  case IconType::NAV_DATABASE:
    return "\uE904";
  case IconType::NAV_PLAYLIST:
    return "\uE90F";
  case IconType::NAV_YTDLP:
    return "\uE919";
  case IconType::NAV_VISUALIZER:
    return "\uE909";

  // UI Actions & Indicators
  case IconType::ADD:
    return "\uE900";
  case IconType::ADD_TO_QUEUE:
  case IconType::ADD_TO_LIST:
    return "\uE901";
  case IconType::REMOVE:
  case IconType::DELETE:
    return "\uE905";
  case IconType::CLEAR:
  case IconType::CLEAR_ALL:
    return "\uE902";
  case IconType::EDIT:
    return "\uE912";
  case IconType::SEARCH:
    return "\uE904";
  case IconType::COPY:
    return "\uE903";
  case IconType::MOVE:
    return "\uE908";
  case IconType::FOLDER:
    return "\uE901";
  case IconType::MUSIC_NOTE:
    return "\uE909";
  case IconType::SETTINGS:
    return "\uE913";
  case IconType::DOWNLOAD:
    return "\uE901";
  case IconType::BACK:
    return "\uE906";
  case IconType::CHECK:
    return "\uE900";
  case IconType::CROSS:
    return "\uE905";
  case IconType::LOADING:
    return "\uE909";
  case IconType::PASTE:
    return "\uE90B";
  case IconType::MENU:
    return "\uE90A";
  case IconType::STREAM:
    return "\uE919";
  case IconType::NETWORK:
    return "\uE919";
  case IconType::REFRESH:
  case IconType::UPDATE_DB:
  case IconType::RESCAN_DB:
    return "\uE904";
  case IconType::SAVE:
    return "\uE901";
  case IconType::CHEVRON_DOWN:
  case IconType::DETAILS:
    return "\uE906";
  case IconType::CHEVRON_UP:
    return "\uE915";
  }

  return "▪";
}

std::string IconProvider::getVolumeIcon(bool muted, int vol) {
  if (muted || vol == 0) {
    return getIcon(IconType::VOLUME_MUTED);
  }
  if (vol > 0 && vol <= 50) {
    return getIcon(IconType::VOLUME_LOW);
  }
  return getIcon(IconType::VOLUME_HIGH);
}

} // namespace UI::Components

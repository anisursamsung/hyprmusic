#include "IconProvider.hpp"
#include "EmbeddedFont.hpp"
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>
#include <filesystem>
#include <fstream>

namespace UI::Components {

static bool g_fontRegistered = false;

void IconProvider::registerCustomFont() {
  if (g_fontRegistered)
    return;

  FcConfig *config = FcConfigGetCurrent();
  if (!config) {
    config = FcInitLoadConfigAndFonts();
    FcConfigSetCurrent(config);
  }

  std::filesystem::path fontPath =
      std::filesystem::temp_directory_path() / "hlmusic_embedded_font.ttf";

  std::ofstream ofs(fontPath, std::ios::binary);
  if (ofs) {
    ofs.write(reinterpret_cast<const char *>(g_hlmusicFontData),
              g_hlmusicFontDataLen);
    ofs.close();

    std::string absPathStr = std::filesystem::absolute(fontPath).string();
    if (FcConfigAppFontAddFile(
            config, reinterpret_cast<const FcChar8 *>(absPathStr.c_str())) ==
        FcTrue) {
      FcConfigBuildFonts(config);
      pango_cairo_font_map_set_default(NULL);
      g_fontRegistered = true;
    }
  }
}

std::string IconProvider::getCustomFontFamily() {
  registerCustomFont();
  return "hyprmusic";
}

bool IconProvider::isCustomFontIcon(IconType) {
  return true;
}

bool IconProvider::isCustomFontIcon(const std::string &) {
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
  case IconType::SHUFFLE_SIMPLE:
    return "\uE920";

  // Playback Modes
  case IconType::REPEAT_OFF:
    return "\uE91D";
  case IconType::REPEAT_ONCE:
    return "\uE91E";
  case IconType::REPEAT_ALL:
    return "\uE91F";
  case IconType::CONSUME:
    return "\uE921";

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
    return "\uE901";
  case IconType::ADD_TO_PLAYLIST:
    return "\uE90E";
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
  case IconType::FOLDER:
    return "\uE908";
  case IconType::MUSIC_NOTE:
    return "\uE909";
  case IconType::SETTINGS:
    return "\uE913";
  case IconType::DOWNLOAD:
    return "\uE91C";
  case IconType::BACK:
    return "\uE91B";
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

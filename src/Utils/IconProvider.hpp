#pragma once
#include <string>

namespace UI::Components {

enum class IconType {
  // Transport Controls
  PLAY,
  PAUSE,
  PREV_TRACK,
  NEXT_TRACK,
  SHUFFLE,
  SHUFFLE_SIMPLE,

  // Playback Modes
  REPEAT_OFF,
  REPEAT_ONCE,
  REPEAT_ALL,
  CONSUME,

  // Volume
  VOLUME_MUTED,
  VOLUME_LOW,
  VOLUME_MEDIUM,
  VOLUME_HIGH,

  // Navigation Tabs
  NAV_QUEUE,
  NAV_DATABASE,
  NAV_PLAYLIST,
  NAV_YTDLP,
  NAV_VISUALIZER,

  // UI Actions & Indicators
  ADD,
  ADD_TO_QUEUE,
  ADD_TO_PLAYLIST,
  REMOVE,
  DELETE,
  CLEAR,
  CLEAR_ALL,
  EDIT,
  SEARCH,
  COPY,
  MOVE,
  FOLDER,
  MUSIC_NOTE,
  SETTINGS,
  DOWNLOAD,
  BACK,
  CHECK,
  CROSS,
  LOADING,
  PASTE,
  MENU,
  STREAM,
  NETWORK,
  REFRESH,
  UPDATE_DB,
  RESCAN_DB,
  SAVE,
  CHEVRON_DOWN,
  CHEVRON_UP,
  DETAILS
};

class IconProvider {
public:
  // Returns the single source of truth icon string for a given IconType.
  static std::string getIcon(IconType type);

  // Dynamic volume helper returning the icon string based on volume and mute state.
  static std::string getVolumeIcon(bool muted, int vol);

  // Register embedded assets/fonts/hyprmusic.ttf with Fontconfig
  static void registerCustomFont();

  // Custom font family name for hyprmusic custom glyphs
  static std::string getCustomFontFamily();
  static bool isCustomFontIcon(IconType type);
  static bool isCustomFontIcon(const std::string &iconStr);
};

} // namespace UI::Components

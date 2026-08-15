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
  REMOVE,
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
  SAVE
};

class IconProvider {
public:
  // Returns the single source of truth icon string for a given IconType.
  static std::string getIcon(IconType type);

  // Dynamic volume helper returning the icon string based on volume and mute state.
  static std::string getVolumeIcon(bool muted, int vol);
};

} // namespace UI::Components

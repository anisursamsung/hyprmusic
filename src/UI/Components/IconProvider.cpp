#include "IconProvider.hpp"

namespace UI::Components {

std::string IconProvider::getIcon(IconType type) {
  switch (type) {
  // Transport Controls
  case IconType::PLAY:
    return "▶\uFE0E";
  case IconType::PAUSE:
    return "⏸\uFE0E";
  case IconType::PREV_TRACK:
    return "⏮\uFE0E";
  case IconType::NEXT_TRACK:
    return "⏭\uFE0E";
  case IconType::SHUFFLE:
    return "⤮";

  // Volume
  case IconType::VOLUME_MUTED:
    return "🔇\uFE0E";
  case IconType::VOLUME_LOW:
    return "🔈\uFE0E";
  case IconType::VOLUME_MEDIUM:
    return "🔉\uFE0E";
  case IconType::VOLUME_HIGH:
    return "🔊\uFE0E";

  // Navigation Tabs
  case IconType::NAV_QUEUE:
    return "☰";
  case IconType::NAV_DATABASE:
    return "🗃";
  case IconType::NAV_PLAYLIST:
    return "♫";
  case IconType::NAV_YTDLP:
    return "⤓";
  case IconType::NAV_VISUALIZER:
    return "☌";

  // UI Actions & Indicators
  case IconType::ADD:
    return "✚";
  case IconType::REMOVE:
    return "🗑";
  case IconType::EDIT:
    return "✏";
  case IconType::SEARCH:
    return "⚲";
  case IconType::COPY:
    return "🗎";
  case IconType::MOVE:
    return "↔";
  case IconType::FOLDER:
    return "🗁";
  case IconType::MUSIC_NOTE:
    return "♫";
  case IconType::SETTINGS:
    return "⚙\uFE0E";
  case IconType::DOWNLOAD:
    return "⤓";
  case IconType::BACK:
    return "◀";
  case IconType::CHECK:
    return "✓";
  case IconType::CROSS:
    return "✕";
  case IconType::LOADING:
    return "⌛";
  case IconType::PASTE:
    return "🗎";
  case IconType::MENU:
    return "⋮";
  case IconType::STREAM:
    return "⬡";
  case IconType::NETWORK:
    return "⬡";
  case IconType::REFRESH:
    return "⟳";
  case IconType::UPDATE_DB:
    return "⟳";
  case IconType::RESCAN_DB:
    return "⚲";
  case IconType::SAVE:
    return "⭳";
  }

  // Fallback icon if type is unrecognized internally
  return "▪";
}

std::string IconProvider::getVolumeIcon(bool muted, int vol) {
  if (muted || vol == 0) {
    return getIcon(IconType::VOLUME_MUTED);
  }
  if (vol > 0 && vol <= 33) {
    return getIcon(IconType::VOLUME_LOW);
  }
  if (vol > 33 && vol <= 66) {
    return getIcon(IconType::VOLUME_MEDIUM);
  }
  return getIcon(IconType::VOLUME_HIGH);
}

} // namespace UI::Components

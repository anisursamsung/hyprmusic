#include "ArtworkUtils.hpp"
#include <mpd/readpicture.h>
#include <mpd/albumart.h>
#include <vector>
#include <fstream>
#include <filesystem>

namespace Utils {

std::string getBackgroundImagePath() {
  std::vector<std::string> candidates = {
      "/home/anisur/git/hyprmusic/assets/background.jpeg",
      "assets/background.jpeg"};
  try {
    candidates.push_back(
        (std::filesystem::current_path() / "assets/background.jpeg")
            .string());
  } catch (...) {
  }
  for (const auto &p : candidates) {
    if (!p.empty() && std::filesystem::exists(p)) {
      return p;
    }
  }
  return "";
}

std::string getDefaultArtworkPath() {
  std::vector<std::string> candidates = {
      "/home/anisur/git/hyprmusic/assets/default_album_art.png",
      "assets/default_album_art.png"};
  try {
    candidates.push_back(
        (std::filesystem::current_path() / "assets/default_album_art.png")
            .string());
  } catch (...) {
  }
  for (const auto &p : candidates) {
    if (!p.empty() && std::filesystem::exists(p)) {
      return p;
    }
  }
  return "";
}

std::string resolveTrackArtwork(struct mpd_connection *conn, const std::string &songUri) {
  if (songUri.empty())
    return getDefaultArtworkPath();

  std::vector<uint8_t> imgBytes;
  char buffer[16384];

  if (conn) {
    // 1. Try mpd_run_readpicture for embedded cover art in track metadata
    unsigned offset = 0;
    int r = 0;
    while ((r = mpd_run_readpicture(conn, songUri.c_str(), offset, buffer, sizeof(buffer))) > 0) {
      imgBytes.insert(imgBytes.end(), buffer, buffer + r);
      offset += r;
    }

    // 2. Try mpd_run_albumart for directory album art (cover.jpg/folder.jpg)
    if (imgBytes.empty()) {
      offset = 0;
      while ((r = mpd_run_albumart(conn, songUri.c_str(), offset, buffer, sizeof(buffer))) > 0) {
        imgBytes.insert(imgBytes.end(), buffer, buffer + r);
        offset += r;
      }
    }
  }

  std::string rawPath = "";

  if (!imgBytes.empty()) {
    std::string tmpPath = "/tmp/hyprmusic_raw_art.bin";
    std::ofstream ofs(tmpPath, std::ios::binary);
    if (ofs) {
      ofs.write(reinterpret_cast<const char *>(imgBytes.data()), imgBytes.size());
      ofs.close();
      rawPath = tmpPath;
    }
  }

  // 3. Fallback: check local directory near song file
  if (rawPath.empty()) {
    try {
      std::filesystem::path songPath(songUri);
      if (std::filesystem::exists(songPath)) {
        auto dir = songPath.parent_path();
        std::vector<std::string> coverNames = {"cover.jpg", "cover.png", "folder.jpg", "folder.png", "album.jpg", "AlbumArtSmall.jpg"};
        for (const auto &name : coverNames) {
          auto coverP = dir / name;
          if (std::filesystem::exists(coverP)) {
            rawPath = coverP.string();
            break;
          }
        }
      }
    } catch (...) {}
  }

  // 4. Default artwork fallback
  if (rawPath.empty()) {
    rawPath = getDefaultArtworkPath();
  }

  return rawPath;
}

} // namespace Utils

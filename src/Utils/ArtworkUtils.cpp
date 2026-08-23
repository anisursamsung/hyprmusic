#include "ArtworkUtils.hpp"
#include "EmbeddedArtwork.hpp"
#include <mpd/readpicture.h>
#include <mpd/albumart.h>
#include <mpd/connection.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>

namespace Utils {

std::string getDefaultArtworkPath() {
  static std::string s_defaultPath = "";
  if (!s_defaultPath.empty() && std::filesystem::exists(s_defaultPath))
    return s_defaultPath;

  std::filesystem::path tempDir = std::filesystem::temp_directory_path();
  std::filesystem::path artPath = tempDir / "hyprmusic_default_album_art.png";

  if (!std::filesystem::exists(artPath)) {
    std::ofstream ofs(artPath, std::ios::binary);
    if (ofs) {
      ofs.write(reinterpret_cast<const char *>(g_defaultAlbumArtData),
                g_defaultAlbumArtDataLen);
      ofs.close();
    }
  }

  s_defaultPath = artPath.string();
  return s_defaultPath;
}

static std::unordered_map<std::string, std::string> s_artworkCache;

std::string getCachedTrackArtwork(const std::string &songUri) {
  if (songUri.empty())
    return "";
  auto it = s_artworkCache.find(songUri);
  if (it != s_artworkCache.end()) {
    return it->second;
  }
  return "";
}

std::string resolveTrackArtwork(struct mpd_connection *conn, const std::string &songUri) {
  if (songUri.empty())
    return getDefaultArtworkPath();

  auto cached = getCachedTrackArtwork(songUri);
  if (!cached.empty())
    return cached;

  // Immediate guard: Stream URLs cannot have embedded/folder album art extracted via MPD or filesystem
  if (songUri.rfind("http://", 0) == 0 || songUri.rfind("https://", 0) == 0 ||
      songUri.find("googlevideo.com") != std::string::npos ||
      songUri.find("youtube.com") != std::string::npos) {
    return getDefaultArtworkPath();
  }

  std::string cleanUri = songUri;
  if (cleanUri.rfind("file://", 0) == 0) {
    cleanUri = cleanUri.substr(7);
  }

  std::vector<uint8_t> imgBytes;
  char buffer[16384];

  if (conn) {
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      mpd_connection_clear_error(conn);
    }

    std::vector<std::string> uriAttempts = {songUri};
    if (cleanUri != songUri) {
      uriAttempts.push_back(cleanUri);
    }

    // 1. Try mpd_run_readpicture for embedded cover art in track metadata
    for (const auto &u : uriAttempts) {
      unsigned offset = 0;
      int r = 0;
      while ((r = mpd_run_readpicture(conn, u.c_str(), offset, buffer, sizeof(buffer))) > 0) {
        imgBytes.insert(imgBytes.end(), buffer, buffer + r);
        offset += r;
      }
      if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        mpd_connection_clear_error(conn);
      }
      if (!imgBytes.empty())
        break;
    }

    // 2. Try mpd_run_albumart for directory album art (cover.jpg/folder.jpg)
    if (imgBytes.empty()) {
      for (const auto &u : uriAttempts) {
        unsigned offset = 0;
        int r = 0;
        while ((r = mpd_run_albumart(conn, u.c_str(), offset, buffer, sizeof(buffer))) > 0) {
          imgBytes.insert(imgBytes.end(), buffer, buffer + r);
          offset += r;
        }
        if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
          mpd_connection_clear_error(conn);
        }
        if (!imgBytes.empty())
          break;
      }
    }
  }

  std::string rawPath = "";
  if (!imgBytes.empty()) {
    std::string ext = ".jpg";
    if (imgBytes.size() >= 4) {
      if (imgBytes[0] == 0x89 && imgBytes[1] == 'P' && imgBytes[2] == 'N' && imgBytes[3] == 'G') {
        ext = ".png";
      } else if (imgBytes[0] == 'R' && imgBytes[1] == 'I' && imgBytes[2] == 'F' && imgBytes[3] == 'F') {
        ext = ".webp";
      } else if (imgBytes[0] == 0xFF && imgBytes[1] == 0xD8) {
        ext = ".jpg";
      }
    }

    std::string tmpPath = "/tmp/hyprmusic_art_" + std::to_string(std::hash<std::string>{}(songUri)) + ext;
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
      std::filesystem::path songPath(cleanUri);
      std::vector<std::filesystem::path> candidates;
      candidates.push_back(songPath);
      const char *home = std::getenv("HOME");
      if (home) {
        candidates.push_back(std::filesystem::path(home) / "Music" / cleanUri);
      }

      for (const auto &p : candidates) {
        std::filesystem::path checkP = p;
        if (!std::filesystem::exists(checkP)) {
          continue;
        }
        auto dir = std::filesystem::is_directory(checkP) ? checkP : checkP.parent_path();
        std::vector<std::string> coverNames = {"cover.jpg", "cover.png", "folder.jpg", "folder.png", "album.jpg", "AlbumArtSmall.jpg"};
        for (const auto &name : coverNames) {
          auto coverP = dir / name;
          if (std::filesystem::exists(coverP)) {
            rawPath = coverP.string();
            break;
          }
        }
        if (!rawPath.empty()) break;
      }
    } catch (...) {}
  }

  // 4. Cache valid non-default artwork; fallback to default if missing
  if (!rawPath.empty() && rawPath != getDefaultArtworkPath()) {
    s_artworkCache[songUri] = rawPath;
  }
  
  if (rawPath.empty()) {
    rawPath = getDefaultArtworkPath();
  }

  return rawPath;
}

} // namespace Utils

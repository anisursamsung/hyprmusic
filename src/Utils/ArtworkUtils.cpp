#include "ArtworkUtils.hpp"
#include <mpd/readpicture.h>
#include <mpd/albumart.h>
#include <mpd/connection.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>

#include <iostream>

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
  static std::string s_defaultPath = "";
  if (!s_defaultPath.empty())
    return s_defaultPath;

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
      s_defaultPath = p;
      return s_defaultPath;
    }
  }
  return "";
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

static std::vector<uint8_t> extractEmbeddedArtworkFromLocalFile(const std::string &filePath) {
  std::vector<uint8_t> result;
  std::string path = filePath;
  if (path.rfind("file://", 0) == 0) {
    path = path.substr(7);
  }

  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
    return result;
  }

  std::ifstream ifs(path, std::ios::binary);
  if (!ifs)
    return result;

  ifs.seekg(0, std::ios::end);
  std::streamsize fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  std::size_t readSize = static_cast<std::size_t>(std::min<std::streamsize>(fileSize, 2097152));
  if (readSize < 10)
    return result;

  std::vector<uint8_t> buffer(readSize);
  ifs.read(reinterpret_cast<char *>(buffer.data()), readSize);

  // 1. ID3v2 APIC/PIC extraction (MP3/AIFF)
  if (buffer.size() >= 10 && buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3') {
    uint32_t tagSize = ((buffer[6] & 0x7F) << 21) | ((buffer[7] & 0x7F) << 14) |
                       ((buffer[8] & 0x7F) << 7) | (buffer[9] & 0x7F);
    std::size_t limit = std::min<std::size_t>(buffer.size(), tagSize + 10);
    std::size_t idx = 10;

    while (idx + 10 < limit) {
      if (buffer[idx] == 0)
        break;

      std::string frameId(reinterpret_cast<char *>(&buffer[idx]), 4);
      uint32_t frameSize = (buffer[idx + 4] << 24) | (buffer[idx + 5] << 16) |
                           (buffer[idx + 6] << 8) | buffer[idx + 7];
      idx += 10;

      if (idx + frameSize > limit)
        break;

      if (frameId == "APIC" && frameSize > 10) {
        std::size_t p = idx + 1;
        while (p < idx + frameSize && buffer[p] != 0)
          p++;
        if (p < idx + frameSize)
          p++;
        if (p < idx + frameSize)
          p++;
        while (p < idx + frameSize && buffer[p] != 0)
          p++;
        if (p < idx + frameSize)
          p++;

        if (p < idx + frameSize) {
          result.assign(buffer.begin() + p, buffer.begin() + idx + frameSize);
          return result;
        }
      }
      idx += frameSize;
    }
  }

  // 2. FLAC METADATA_BLOCK_PICTURE (block type 6)
  if (buffer.size() >= 4 && buffer[0] == 'f' && buffer[1] == 'L' && buffer[2] == 'a' && buffer[3] == 'C') {
    std::size_t idx = 4;
    while (idx + 4 < buffer.size()) {
      bool isLast = (buffer[idx] & 0x80) != 0;
      uint8_t type = buffer[idx] & 0x7F;
      uint32_t blockSize = (buffer[idx + 1] << 16) | (buffer[idx + 2] << 8) | buffer[idx + 3];
      idx += 4;

      if (idx + blockSize > buffer.size())
        break;

      if (type == 6 && blockSize > 32) {
        std::size_t p = idx + 4;
        if (p + 4 <= idx + blockSize) {
          uint32_t mimeLen = (buffer[p] << 24) | (buffer[p + 1] << 16) | (buffer[p + 2] << 8) | buffer[p + 3];
          p += 4 + mimeLen;
        }
        if (p + 4 <= idx + blockSize) {
          uint32_t descLen = (buffer[p] << 24) | (buffer[p + 1] << 16) | (buffer[p + 2] << 8) | buffer[p + 3];
          p += 4 + descLen;
        }
        p += 16;
        if (p + 4 <= idx + blockSize) {
          uint32_t dataLen = (buffer[p] << 24) | (buffer[p + 1] << 16) | (buffer[p + 2] << 8) | buffer[p + 3];
          p += 4;
          if (p + dataLen <= idx + blockSize) {
            result.assign(buffer.begin() + p, buffer.begin() + p + dataLen);
            return result;
          }
        }
      }

      if (isLast)
        break;
      idx += blockSize;
    }
  }

  // 3. General Magic Header scan for embedded JPEG or PNG image bytes
  for (std::size_t i = 0; i + 8 < buffer.size(); ++i) {
    if (buffer[i] == 0xFF && buffer[i + 1] == 0xD8 && buffer[i + 2] == 0xFF) {
      for (std::size_t j = i + 4; j + 1 < buffer.size(); ++j) {
        if (buffer[j] == 0xFF && buffer[j + 1] == 0xD9) {
          result.assign(buffer.begin() + i, buffer.begin() + j + 2);
          return result;
        }
      }
    }
    if (buffer[i] == 0x89 && buffer[i + 1] == 'P' && buffer[i + 2] == 'N' && buffer[i + 3] == 'G' &&
        buffer[i + 4] == 0x0D && buffer[i + 5] == 0x0A && buffer[i + 6] == 0x1A && buffer[i + 7] == 0x0A) {
      for (std::size_t j = i + 8; j + 12 <= buffer.size(); ++j) {
        if (buffer[j + 4] == 'I' && buffer[j + 5] == 'E' && buffer[j + 6] == 'N' && buffer[j + 7] == 'D') {
          result.assign(buffer.begin() + i, buffer.begin() + j + 12);
          return result;
        }
      }
    }
  }

  return result;
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

  std::vector<uint8_t> imgBytes;
  char buffer[16384];

  if (conn) {
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      mpd_connection_clear_error(conn);
    }

    // 1. Try mpd_run_readpicture for embedded cover art in track metadata
    unsigned offset = 0;
    int r = 0;
    while ((r = mpd_run_readpicture(conn, songUri.c_str(), offset, buffer, sizeof(buffer))) > 0) {
      imgBytes.insert(imgBytes.end(), buffer, buffer + r);
      offset += r;
    }
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
      mpd_connection_clear_error(conn);
    }

    // 2. Try mpd_run_albumart for directory album art (cover.jpg/folder.jpg)
    if (imgBytes.empty()) {
      offset = 0;
      while ((r = mpd_run_albumart(conn, songUri.c_str(), offset, buffer, sizeof(buffer))) > 0) {
        imgBytes.insert(imgBytes.end(), buffer, buffer + r);
        offset += r;
      }
      if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        mpd_connection_clear_error(conn);
      }
    }
  }

  // 3. Fallback: Directly extract embedded cover art from local file if MPD picture read returned empty
  if (imgBytes.empty()) {
    std::string checkPath = songUri;
    if (checkPath.rfind("file://", 0) == 0) {
      checkPath = checkPath.substr(7);
    }
    if (std::filesystem::exists(checkPath)) {
      imgBytes = extractEmbeddedArtworkFromLocalFile(checkPath);
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
      std::filesystem::path songPath(songUri);
      std::vector<std::filesystem::path> candidates;
      candidates.push_back(songPath);
      const char *home = std::getenv("HOME");
      if (home) {
        candidates.push_back(std::filesystem::path(home) / "Music" / songUri);
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

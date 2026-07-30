#include "ArtworkUtils.hpp"
#include <mpd/readpicture.h>
#include <mpd/albumart.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace Utils {

static std::string applyRoundedCorners(const std::string &inputPath, int width, int height, int cornerRadius) {
  if (inputPath.empty() || !std::filesystem::exists(inputPath))
    return "";

  std::string outputPath = "/tmp/hyprmusic_rounded_art.png";
  std::string cmd = "magick \"" + inputPath + "\" -resize " + std::to_string(width) + "x" + std::to_string(height) +
                    "^ -gravity center -extent " + std::to_string(width) + "x" + std::to_string(height) +
                    " \\( +clone -alpha extract -draw \"roundrectangle 0,0 " + std::to_string(width) + "," + std::to_string(height) +
                    " " + std::to_string(cornerRadius) + "," + std::to_string(cornerRadius) + "\" \\)" +
                    " -alpha off -compose CopyOpacity -composite \"" + outputPath + "\" >/dev/null 2>&1";

  int res = std::system(cmd.c_str());
  if (res == 0 && std::filesystem::exists(outputPath)) {
    return outputPath;
  }
  return inputPath;
}

std::string resolveTrackArtwork(struct mpd_connection *conn, const std::string &songUri, int targetWidth, int targetHeight, int cornerRadius) {
  if (songUri.empty())
    return "";

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

  if (!rawPath.empty() && cornerRadius > 0) {
    std::string processed = applyRoundedCorners(rawPath, targetWidth, targetHeight, cornerRadius);
    if (!processed.empty())
      return processed;
  }

  return rawPath;
}

} // namespace Utils

#pragma once
#include <mpd/client.h>
#include <string>

namespace Utils {

/**
 * Resolves artwork image for a song URI.
 * First checks embedded metadata picture via MPD (mpd_run_readpicture).
 * Next checks directory cover art via MPD (mpd_run_albumart).
 * Finally checks local filesystem parent folder for cover art images.
 * Returns file path to the resolved image, or empty string if not found.
 */
std::string resolveTrackArtwork(struct mpd_connection *conn, const std::string &songUri, int targetWidth = 450, int targetHeight = 350, int cornerRadius = 0);

} // namespace Utils

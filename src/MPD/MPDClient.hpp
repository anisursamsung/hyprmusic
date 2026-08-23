#pragma once
#include <mpd/client.h>
#include <string>
#include <unordered_set>

namespace MPDUtils {
std::unordered_set<std::string> getQueueUris(struct mpd_connection *conn);
std::unordered_set<std::string> getPlaylistUris(struct mpd_connection *conn, const std::string &playlistName);
}

#include "MPDClient.hpp"

namespace MPDUtils {

std::unordered_set<std::string> getQueueUris(struct mpd_connection *conn) {
  std::unordered_set<std::string> uris;
  if (conn && mpd_send_list_queue_meta(conn)) {
    struct mpd_song *s;
    while ((s = mpd_recv_song(conn)) != NULL) {
      const char *uri = mpd_song_get_uri(s);
      if (uri) {
        uris.insert(uri);
      }
      mpd_song_free(s);
    }
    mpd_response_finish(conn);
  }
  return uris;
}

std::unordered_set<std::string> getPlaylistUris(struct mpd_connection *conn, const std::string &playlistName) {
  std::unordered_set<std::string> uris;
  if (conn && !playlistName.empty() && mpd_send_list_playlist_meta(conn, playlistName.c_str())) {
    struct mpd_song *s;
    while ((s = mpd_recv_song(conn)) != NULL) {
      const char *uri = mpd_song_get_uri(s);
      if (uri) {
        uris.insert(uri);
      }
      mpd_song_free(s);
    }
    mpd_response_finish(conn);
  }
  return uris;
}

} // namespace MPDUtils

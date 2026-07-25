# HyprMusic

Hyprland specific music player front end for Media Player Daemon (mpd) in Linux. Uses Hyprtoolkit API and C++.
---

## 🎵 Key Features & Tabs

### 1. Queue Tab
- **Current Playback**: View all tracks currently in MPD playback queue.
- **Track Controls**: Play, pause, remove, or jump to any track.
- **Search Filter**: Instantly search across queue track titles, artists, and albums.
- **Queue Actions**: Add items from Que to playlist, remove item from Que.
- **Add item to Que**: Add item to que e.g. A stream link, from database, from playlist.
- **Streaming**: Add stream link directly to que.

### 2. Database Tab
- Lists all the tracks in the database which in MPD language means all the tracks in the Music Directory that is pointed in mpd.conf file. Default is ~/Music.
- Items from database can be added to Que or any Playlist.
### 3. Playlists Tab
- **Playlist Management**: View all saved MPD playlists in your library.
- **Creation & Removal**: Create new empty playlists or delete existing ones.
- **Track Inspection**: Expand any playlist to inspect track lists and load them into your current queue.

### 4. YT-DLP Online Search Tab
- View and edit the settings you have configured in ~/.config/mpd.conf
## ⌨️ Playback & Control Bar

- **Bottom Bar**: Contains track title, artist, elapsed/total time, interactive seek bar, play/pause toggle, previous/next controls, and volume slider.
- **MPD Backend**: Playback continues smoothly in the background even if the UI is closed.

# HlMusic

A dedicated Hyprland frontend for the Media Player Daemon (MPD) on Linux, built with C++ and the Hyprtoolkit API.

---

## 🎵 Key Features & Tabs

### 1. Queue Tab
* **Current Playback:** Monitor all tracks currently in the MPD playback queue.
* **Track Controls:** Play, pause, remove, or jump directly to any specific track.
* **Search Filter:** Instantly filter the queue by track title, artist, or album.
* **Queue Management:** Seamlessly add items from the queue to a playlist, or remove them entirely.
* **Versatile Additions:** Add items to the queue from various sources, including your database, existing playlists, or direct stream links.

### 2. Database Tab
* **Library Overview:** View all tracks stored in your database (all files within the music directory specified in `mpd.conf`, defaulting to `~/Music`).
* **Seamless Integration:** Quickly play or append any item from the database to your active queue or a saved playlist.
* **Database Updates:** Trigger quick MPD database updates or complete rescan commands directly from the interface.

### 3. Playlists Tab
* **Playlist Management:** Access and view all saved MPD playlists in your library via grid cards or detailed view.
* **Creation, Renaming & Deletion:** Generate new empty playlists, rename existing playlists, or delete unwanted ones.
* **Track Inspection:** Expand individual playlists to inspect their track listing, reorder/move tracks, or load them into the active playback queue.

### 4. YT-DLP Online Search Tab
* **Online Search:** Search YouTube audio tracks online via `yt-dlp` integration.
* **Direct Streaming:** Stream YouTube audio directly into MPD or import entire YouTube playlists.
* **Flexible Actions:** Add online search results directly to your queue or save them into local MPD playlists.

### 5. Settings Tab
* **Configuration Management:** View and edit MPD server settings (directories, network bind/port, auto-update, PipeWire Pulse audio output, FIFO visualizer).
* **Save & Restart:** Persist configuration updates to `~/.config/mpd/mpd.conf` and restart the MPD service.

### 6. Help Tab
* **User Guide:** Access built-in documentation and guide overview directly within the application.

---

## ⌨️ Playback & Control Bar

* **Persistent Control Bar:** The bottom bar provides quick access to track information (title, artist, stream status), elapsed and total time, an interactive seek slider, play/pause toggle, previous/next controls, volume slider, and playback modes (Shuffle, Repeat, Consume).
* **Background Execution:** Powered by the MPD backend, audio playback continues uninterrupted in the background even if the graphical interface is closed.

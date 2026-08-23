cmake_minimum_required(VERSION 3.19)

project(hyprmusic VERSION 1.0 DESCRIPTION "HyprMusic Player")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-O3 -Wall -Wextra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET hyprtoolkit pixman-1 libdrm fontconfig pangocairo)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)
pkg_check_modules(MPDCLIENT REQUIRED IMPORTED_TARGET libmpdclient)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(EMBEDDED_FONT_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedFont.cpp")
set(EMBEDDED_ARTWORK_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedArtwork.cpp")

add_custom_command(
    OUTPUT ${EMBEDDED_FONT_CPP}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/hyprmusic.ttf
            ${EMBEDDED_FONT_CPP}
            UI::Components g_hyprmusicFontData
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/hyprmusic.ttf
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
    COMMENT "Auto-generating EmbeddedFont.cpp from assets/fonts/hyprmusic.ttf"
)

add_custom_command(
    OUTPUT ${EMBEDDED_ARTWORK_CPP}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/default_album_art.png
            ${EMBEDDED_ARTWORK_CPP}
            Utils g_defaultAlbumArtData
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/default_album_art.png
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
    COMMENT "Auto-generating EmbeddedArtwork.cpp from assets/default_album_art.png"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(hyprmusic 
    src/main.cpp
    src/Core/HyprMusicApp.cpp
    src/Core/IpcService.cpp

    src/MPD/MPDClient.cpp
    src/MPD/MPDManager.cpp

    src/PlaybackBar/PlaybackBar.cpp
    src/PlaybackBar/MiniArt/MiniArtworkCard.cpp
    src/PlaybackBar/Subsections/Common/CustomSeekBar.cpp
    src/PlaybackBar/Subsections/TopSection.cpp
    src/PlaybackBar/Subsections/MiddleSection.cpp
    src/PlaybackBar/Subsections/BottomSection.cpp

    src/Tabs/Shared/SongCard.cpp
    src/Tabs/Queue/QueueView.cpp
    src/Tabs/Database/DatabaseView.cpp
    src/Tabs/Playlists/PlaylistsView.cpp
    src/Tabs/Player/PlayerView.cpp
    src/Tabs/YtDlp/YtDlpView.cpp
    src/Tabs/YtDlp/YtDlpService.cpp
    src/Tabs/Settings/SettingsView.cpp
    src/Tabs/Settings/SettingsManager.cpp
    src/Tabs/Visualizer/VisualizerView.cpp
    src/Tabs/Visualizer/Visualizer.cpp
    src/Tabs/Visualizer/Visualizations/RainbowSpectrumVisualization.cpp
    src/Tabs/Visualizer/Visualizations/PeakHoldNeonVisualization.cpp
    src/Tabs/Visualizer/Visualizations/LiquidWavyVisualization.cpp
    src/Tabs/Visualizer/Visualizations/StereoMirrorWaveVisualization.cpp
    src/Tabs/Visualizer/Visualizations/CircularRadialVisualization.cpp
    src/Tabs/Visualizer/Visualizations/DotMatrixVisualization.cpp

    src/Dialogs/BaseDialog.cpp
    src/Dialogs/PromptDialog.cpp
    src/Dialogs/ConfirmDialog.cpp
    src/Dialogs/ActionMenuDialog.cpp
    src/Dialogs/AddItemDialog.cpp
    src/Dialogs/DialogCoordinator.cpp
    src/Dialogs/DownloadProgressDialog.cpp
    src/Dialogs/PlaylistSelectionDialog.cpp

    src/Utils/UIFactory.cpp
    src/Utils/IconProvider.cpp
    src/Utils/ArtworkUtils.cpp
    src/Utils/NotificationManager.cpp
    src/Utils/StreamUtils.cpp

    ${EMBEDDED_FONT_CPP}
    ${EMBEDDED_ARTWORK_CPP}
)

target_link_libraries(hyprmusic 
    PkgConfig::DEPS
    PkgConfig::HYPRUTILS
    PkgConfig::MPDCLIENT
)

include(GNUInstallDirs)
install(TARGETS hyprmusic DESTINATION ${CMAKE_INSTALL_BINDIR})
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hyprmusic.desktop")
    install(FILES hyprmusic.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

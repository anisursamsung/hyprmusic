cmake_minimum_required(VERSION 3.19)

project(toplayer VERSION 1.0 DESCRIPTION "HyprMusic Player")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-O3 -Wall -Wextra)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET hyprtoolkit pixman-1 libdrm)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)
pkg_check_modules(MPDCLIENT REQUIRED IMPORTED_TARGET libmpdclient)

add_executable(hyprmusic 
    src/main.cpp
    src/MPDClient.cpp
    src/Core/HyprMusicApp.cpp
    src/Services/MPDManager.cpp
    src/Services/SettingsManager.cpp
    src/Services/YtDlpService.cpp
    src/UI/Components/NotificationManager.cpp
    src/UI/Components/PlaybackBar.cpp
    src/UI/Components/TabBar.cpp
    src/UI/Components/UIFactory.cpp
    src/UI/Dialogs/ActionMenuDialog.cpp
    src/UI/Dialogs/AddItemDialog.cpp
    src/UI/Dialogs/CreatePlaylistDialog.cpp
    src/UI/Dialogs/PlaylistSelectionDialog.cpp
    src/UI/Dialogs/RenamePlaylistDialog.cpp
    src/UI/Views/DatabaseView.cpp
    src/UI/Views/HelpView.cpp
    src/UI/Views/PlaylistsView.cpp
    src/UI/Views/QueueView.cpp
    src/UI/Views/SettingsView.cpp
    src/UI/Views/YtDlpView.cpp
    src/Utils/StreamUtils.cpp
)

target_link_libraries(hyprmusic 
    PkgConfig::DEPS
    PkgConfig::HYPRUTILS
    PkgConfig::MPDCLIENT
)

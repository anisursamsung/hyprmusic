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

# FIX 1: Added IMPORTED_TARGET so it behaves exactly like the ones above
pkg_check_modules(MPDCLIENT REQUIRED IMPORTED_TARGET libmpdclient)

add_executable(hyprmusic 
    src/main.cpp
    src/MPDClient.cpp
    src/Utils/StreamUtils.cpp
    src/UI/Components/NotificationManager.cpp
    src/UI/Dialogs/AddItemDialog.cpp
)

target_link_libraries(hyprmusic 
    PkgConfig::DEPS
    PkgConfig::HYPRUTILS
    PkgConfig::MPDCLIENT
)

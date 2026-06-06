cmake_minimum_required(VERSION 3.21)

project(PokeredSaveEditor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Silence the harmless, very noisy "'dllimport' attribute ignored" warning that
# llvm-mingw/clang emits for the DB_AUTOPORT/SAVEFILE_AUTOPORT QObject structs
# (e.g. MapDBEntry). It's a known cosmetic warning on Qt + mingw shared libs and
# the symbols still resolve correctly. Suppressing it keeps the build log usable.
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wno-ignored-attributes)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 QuickWidgets Svg Qml Charts)
qt_standard_project_setup()

add_subdirectory(common)
add_subdirectory(db)
add_subdirectory(savefile)
add_subdirectory(app)

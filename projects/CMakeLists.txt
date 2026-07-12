cmake_minimum_required(VERSION 3.21)

# --- Version: single source of truth is the repo-root VERSION ---------------
# (See notes/reference/versioning.md.) Parse it BEFORE project() so the parsed
# core number is what project(VERSION) records.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/PSEVersion.cmake)
pse_read_version(${CMAKE_CURRENT_LIST_DIR}/../VERSION)

project(PokeredSaveEditor VERSION ${PSE_VERSION_CORE} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clang-tidy / clangd / cppcheck can see the exact
# per-file flags. Harmless for normal builds; consumed by scripts/lint.* and the
# `lint` CI workflow. See notes/plans/testing.md -> "Static analysis / linting".
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 QuickWidgets Svg Qml Charts Network Test Multimedia LinguistTools)
qt_standard_project_setup()

# --- Version header generation ----------------------------------------------
# Generate pse_version.h (numeric + human SemVer + git build metadata) into the
# build tree. A configure-time pass creates it so the first compile has it; a
# build-time custom target refreshes the git metadata on every rebuild (it only
# rewrites the header when the contents change, so no needless recompiles).
set(PSE_VERSION_HEADER_IN  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pse_version.h.in")
set(PSE_VERSION_HEADER_DIR "${CMAKE_BINARY_DIR}/generated")
set(PSE_VERSION_HEADER     "${PSE_VERSION_HEADER_DIR}/pse_version.h")
file(MAKE_DIRECTORY "${PSE_VERSION_HEADER_DIR}")

pse_git_metadata("${CMAKE_SOURCE_DIR}" PSE_GIT_HASH PSE_GIT_DIRTY)
pse_compose_full("${PSE_VERSION_HUMAN}" "${PSE_GIT_HASH}" "${PSE_GIT_DIRTY}" PSE_VERSION_FULL)
if(PSE_GIT_DIRTY STREQUAL "")
  set(PSE_GIT_DIRTY_BOOL 0)
else()
  set(PSE_GIT_DIRTY_BOOL 1)
endif()
configure_file("${PSE_VERSION_HEADER_IN}" "${PSE_VERSION_HEADER}" @ONLY)

add_custom_target(pse_version_gen
  BYPRODUCTS "${PSE_VERSION_HEADER}"
  COMMAND ${CMAKE_COMMAND}
          -DPSE_MODULE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/cmake
          -DPSE_SRC_DIR=${CMAKE_SOURCE_DIR}
          -DPSE_VERSION_MAJOR=${PSE_VERSION_MAJOR}
          -DPSE_VERSION_MINOR=${PSE_VERSION_MINOR}
          -DPSE_VERSION_PATCH=${PSE_VERSION_PATCH}
          -DPSE_VERSION_HUMAN=${PSE_VERSION_HUMAN}
          -DPSE_HEADER_IN=${PSE_VERSION_HEADER_IN}
          -DPSE_HEADER_OUT=${PSE_VERSION_HEADER}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PSEVersionGen.cmake
  COMMENT "PSEVersion: refreshing version (${PSE_VERSION_HUMAN}) build metadata"
  VERBATIM)

# Windows: embed a VERSIONINFO resource in the .exe (toggle off if your kit's
# RC compiler chokes: -DPSE_GEN_WINRC=OFF).
option(PSE_GEN_WINRC "Embed a Windows VERSIONINFO resource in the .exe" ON)
if(WIN32 AND PSE_GEN_WINRC)
  enable_language(RC)
endif()

add_subdirectory(common)
add_subdirectory(db)
add_subdirectory(savefile)
add_subdirectory(audio)
add_subdirectory(app)

# Test suite (QtTest + CTest). See notes/plans/testing.md. Toggle off with
# -DPSE_BUILD_TESTS=OFF if you only want to build the app. Building "all" builds
# the tests too; building just the app target does not.
option(PSE_BUILD_TESTS "Build the PSE test suite (QtTest + CTest)" ON)
if(PSE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

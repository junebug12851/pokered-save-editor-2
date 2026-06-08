# ---------------------------------------------------------------------------
# PSE test suite (QtTest + CTest)
#
# Phase 1 of notes/plans/testing.md. Each test is its own QtTest executable,
# registered with CTest so the whole suite runs via `ctest`.
#
# Tests link the real libraries (common/db/savefile). `savefile` PUBLIC-links
# `db` and `common`, so linking `savefile` pulls in everything (headers + libs).
# DB::inst() self-initialises its embedded JSON resources, so tests need no
# working-directory assets for the game databases.
#
# Fixtures (the real .sav files) live in <repo>/assets and are located at runtime
# via the PSE_ASSETS_DIR compile definition below. Tests NEVER write to those
# originals -- they operate on in-memory copies.
# ---------------------------------------------------------------------------

# Absolute path to <repo>/assets (this file is at <repo>/projects/tests).
get_filename_component(PSE_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../assets" ABSOLUTE)

# Helper: define one QtTest executable + CTest registration.
#   pse_add_test(<name> <lib> <sources...>)
# <lib> is the primary library to link (e.g. savefile or db); Qt6::Test is always added.
function(pse_add_test name lib)
    qt_add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE ${lib} Qt6::Test)
    target_compile_definitions(${name} PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

pse_add_test(tst_roundtrip    savefile savefile/tst_roundtrip.cpp)
pse_add_test(tst_fields       savefile savefile/tst_fields.cpp)
pse_add_test(tst_pokemon      savefile savefile/tst_pokemon.cpp)
pse_add_test(tst_items        savefile savefile/tst_items.cpp)
pse_add_test(tst_world        savefile savefile/tst_world.cpp)
pse_add_test(tst_verbs        savefile savefile/tst_verbs.cpp)
pse_add_test(tst_toolset      savefile savefile/tst_toolset.cpp)
pse_add_test(tst_common       common   common/tst_common.cpp)
pse_add_test(tst_db_integrity db       db/tst_db_integrity.cpp)

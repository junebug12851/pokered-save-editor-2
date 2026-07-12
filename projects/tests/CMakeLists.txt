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
    # Record the target so the `tests_all` aggregate (below) always covers every
    # suite -- CI builds `tests_all`, so a new test can never be left unbuilt
    # (which would make ctest report it "Not Run" and fail the run).
    set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS ${name})
endfunction()

# One-shot generator for assets/saves/synthetic-clean/ (NOT a CTest test). Build + run manually to
# regenerate the synthetic fixtures; the output .sav files are committed.
qt_add_executable(gen_synthetic_fixtures tools/gen_synthetic_fixtures.cpp)
target_link_libraries(gen_synthetic_fixtures PRIVATE savefile)
target_compile_definitions(gen_synthetic_fixtures PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")

pse_add_test(tst_roundtrip    savefile savefile/tst_roundtrip.cpp)
pse_add_test(tst_fields       savefile savefile/tst_fields.cpp)
pse_add_test(tst_pokemon      savefile savefile/tst_pokemon.cpp)
pse_add_test(tst_pokemon_logic savefile savefile/tst_pokemon_logic.cpp)
pse_add_test(tst_pokemonbox   savefile savefile/tst_pokemonbox.cpp)
pse_add_test(tst_sprite_data  savefile savefile/tst_sprite_data.cpp)
pse_add_test(tst_area_logic   savefile savefile/tst_area_logic.cpp)
pse_add_test(tst_items        savefile savefile/tst_items.cpp)
pse_add_test(tst_items_logic  savefile savefile/tst_items_logic.cpp)
pse_add_test(tst_world        savefile savefile/tst_world.cpp)
pse_add_test(tst_area         savefile savefile/tst_area.cpp)
pse_add_test(tst_area_fragments savefile savefile/tst_area_fragments.cpp)
pse_add_test(tst_sign_data    savefile savefile/tst_sign_data.cpp)
pse_add_test(tst_map_fragments savefile savefile/tst_map_fragments.cpp)

# The sound chip + the sound engine. Links `db` too: the imported music data is baked into db.qrc.
pse_add_test(tst_audio audio audio/tst_audio.cpp)
target_link_libraries(tst_audio PRIVATE db)
pse_add_test(tst_iterator     savefile savefile/tst_iterator.cpp)
pse_add_test(tst_item         savefile savefile/tst_item.cpp)
pse_add_test(tst_player_basics savefile savefile/tst_player_basics.cpp)
pse_add_test(tst_filemanagement savefile savefile/tst_filemanagement.cpp)
pse_add_test(tst_storage      savefile savefile/tst_storage.cpp)
pse_add_test(tst_area_pokemon savefile savefile/tst_area_pokemon.cpp)
pse_add_test(tst_bench        savefile savefile/tst_bench.cpp)
pse_add_test(tst_regression   savefile savefile/tst_regression.cpp)
pse_add_test(tst_fuzz         savefile savefile/tst_fuzz.cpp)
pse_add_test(tst_pokedex      savefile savefile/tst_pokedex.cpp)
pse_add_test(tst_misc_regions savefile savefile/tst_misc_regions.cpp)
pse_add_test(tst_verbs        savefile savefile/tst_verbs.cpp)
pse_add_test(tst_toolset      savefile savefile/tst_toolset.cpp)
pse_add_test(tst_signals      savefile savefile/tst_signals.cpp)
pse_add_test(tst_errors       savefile savefile/tst_errors.cpp)
pse_add_test(tst_dirty_fixtures savefile savefile/tst_dirty_fixtures.cpp)
pse_add_test(tst_synthetic_fixtures savefile savefile/tst_synthetic_fixtures.cpp)
pse_add_test(tst_e2e          savefile savefile/tst_e2e.cpp)
pse_add_test(tst_randomizer   savefile savefile/tst_randomizer.cpp)
pse_add_test(tst_common       common   common/tst_common.cpp)
pse_add_test(tst_common_random common  common/tst_common_random.cpp)
pse_add_test(tst_common_qml    common   common/tst_common_qml.cpp)
pse_add_test(tst_db_integrity db       db/tst_db_integrity.cpp)
pse_add_test(tst_db_entries   db       db/tst_db_entries.cpp)
pse_add_test(tst_db_stores    db       db/tst_db_stores.cpp)
pse_add_test(tst_db_entry_getters db   db/tst_db_entry_getters.cpp)
pse_add_test(tst_db_entry_getters2 db  db/tst_db_entry_getters2.cpp)
pse_add_test(tst_db_coverage_fill db   db/tst_db_coverage_fill.cpp)
pse_add_test(tst_fonts        db       db/tst_fonts.cpp)
pse_add_test(tst_fontsearch   db       db/tst_fontsearch.cpp)
pse_add_test(tst_fontsdb      db       db/tst_fontsdb.cpp)
pse_add_test(tst_mapsearch    db       db/tst_mapsearch.cpp)
pse_add_test(tst_mapsearch_predicates db db/tst_mapsearch_predicates.cpp)
pse_add_test(tst_db_map_subentries db  db/tst_db_map_subentries.cpp)
pse_add_test(tst_db_random_strings db  db/tst_db_random_strings.cpp)

# App-layer tests link the appcore static library (extracted from the app exe).
if(TARGET appcore)
    pse_add_test(tst_models   appcore  mvc/tst_models.cpp)
    pse_add_test(tst_select_models appcore mvc/tst_select_models.cpp)
    pse_add_test(tst_bridge   appcore  mvc/tst_bridge.cpp)
    pse_add_test(tst_tileset_engine appcore mvc/tst_tileset_engine.cpp)
    pse_add_test(tst_storage_model appcore mvc/tst_storage_model.cpp)
    pse_add_test(tst_market_model appcore mvc/tst_market_model.cpp)
    pse_add_test(tst_item_exchange appcore mvc/tst_item_exchange.cpp)
    pse_add_test(tst_item_storage_model appcore mvc/tst_item_storage_model.cpp)
    pse_add_test(tst_move_select_model appcore mvc/tst_move_select_model.cpp)
    pse_add_test(tst_font_keyboard appcore mvc/tst_font_keyboard.cpp)
    pse_add_test(tst_app_e2e  appcore  mvc/tst_app_e2e.cpp)
    pse_add_test(tst_shortcuts appcore mvc/tst_shortcuts.cpp)
    pse_add_test(tst_model_tester appcore mvc/tst_model_tester.cpp)

    # The graphics engine + QML image providers need a GUI app (QPixmap/QPainter)
    # and the app's qrc (the tileset/font PNGs live in app.qrc, not appcore), and
    # must run on the offscreen platform. So it's built specially (qrc compiled in,
    # QTEST_MAIN, offscreen env) rather than via pse_add_test.
    qt_add_executable(tst_engine_providers
        mvc/tst_engine_providers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/app.qrc)
    target_link_libraries(tst_engine_providers PRIVATE appcore Qt6::Test)
    target_compile_definitions(tst_engine_providers PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
    add_test(NAME tst_engine_providers COMMAND tst_engine_providers)
    set_tests_properties(tst_engine_providers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_engine_providers)

    # The map emulator (MapEngine/MapProvider/MapModel + the block data). Same wiring
    # as tst_engine_providers -- it renders real tiles, so it needs a GUI app and the
    # tileset PNGs out of app.qrc, and runs offscreen.
    qt_add_executable(tst_map
        mvc/tst_map.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/app.qrc)
    target_link_libraries(tst_map PRIVATE appcore Qt6::Test)
    target_compile_definitions(tst_map PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
    add_test(NAME tst_map COMMAND tst_map)
    set_tests_properties(tst_map PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_map)

    # EMULATOR PARITY -- the editor judged against the actual Game Boy.
    #
    # Boots the real ROM with one of our saves (via scripts/emu/dump_state.py + PyBoy),
    # reads the console's own RAM back out, and demands MapEngine match it byte for byte:
    # the view pointer, the whole block buffer, the scratch area, and the 20x18 screen.
    #
    # LOCAL-ONLY BY CONSTRUCTION. It needs assets/references/backup.gb -- a legally
    # backed-up cartridge dump that is gitignored and is NEVER committed or shipped.
    # Without the ROM (CI, a fresh clone, anyone else) every case SKIPs and the suite
    # stays green. Nothing here copies the ROM anywhere but the git-ignored tmp/emu.
    qt_add_executable(tst_emu_parity
        emu/tst_emu_parity.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/app.qrc)
    target_link_libraries(tst_emu_parity PRIVATE appcore Qt6::Test)
    target_compile_definitions(tst_emu_parity PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
    add_test(NAME tst_emu_parity COMMAND tst_emu_parity)
    set_tests_properties(tst_emu_parity PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_emu_parity)

    # The SOUND engine, judged the same way. Gen 1's whole sequencer state is 243 bytes at $C000,
    # so the emulator can photograph it every frame while the real cartridge plays a track, and we
    # demand our port -- seeded from the console's own state -- evolve identically. Same ROM rules:
    # local-only, SKIPs without it. It boots the emulator once per track, so it is SLOW by nature.
    qt_add_executable(tst_sound_parity emu/tst_sound_parity.cpp)
    target_link_libraries(tst_sound_parity PRIVATE audio db Qt6::Test)
    target_compile_definitions(tst_sound_parity PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
    add_test(NAME tst_sound_parity COMMAND tst_sound_parity)
    set_tests_properties(tst_sound_parity PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
                                                     TIMEOUT 900)
    set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_sound_parity)
endif()

# --- QML smoke test (Qt Quick Test, headless via offscreen) ---
# Establishes the QML test harness. Screen-level tests that use `brg.*` need the
# Bridge/DB booted into the engine (future work). Guarded so a kit without the
# QuickTest module still configures.
find_package(Qt6 QUIET OPTIONAL_COMPONENTS QuickTest Quick)
if(TARGET Qt6::QuickTest)
    qt_add_executable(tst_qml qml/main.cpp)
    target_link_libraries(tst_qml PRIVATE Qt6::QuickTest Qt6::Qml Qt6::Quick)
    target_compile_definitions(tst_qml PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml/cases")
    add_test(NAME tst_qml COMMAND tst_qml)
    set_tests_properties(tst_qml PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_qml)

    # Behavioural QML test: boots a real Bridge over BaseSAV and exposes it as `brg`,
    # then drives the C++<->QML property chain from QML (the "undefined chain" guard).
    # Needs appcore (the Bridge), so it's nested in the appcore guard too.
    if(TARGET appcore)
        qt_add_executable(tst_qml_brg qml/main_brg.cpp)
        target_link_libraries(tst_qml_brg PRIVATE appcore Qt6::QuickTest Qt6::Qml Qt6::Quick Qt6::Test)
        target_compile_definitions(tst_qml_brg PRIVATE
            QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml/brg_cases"
            PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
        add_test(NAME tst_qml_brg COMMAND tst_qml_brg)
        set_tests_properties(tst_qml_brg PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_qml_brg)

        # QML SCREEN SMOKE TEST: loads EVERY registered screen through a real engine
        # (brg + the two image providers + DB::qmlProtect + the exe's full
        # bootQmlLinkage type registration) and FAILS on any QML warning/error.
        # Catches the load/runtime QML-bug class the C++ suite can't see (e.g. the
        # 2026-06-13 Credits FINAL-override + `id: top` anchor-collision). A QTEST_MAIN
        # GUI test (not a QUICK_TEST), so it's built directly rather than via the
        # qmltest macros. It compiles in:
        #   - app.qrc            -> qrc:/ui/app/... screens + qtquickcontrols2.conf (Material style)
        #   - bootQmlLinkage.cpp -> the SAME QML type registration the exe uses (one source of truth)
        qt_add_executable(tst_qml_screens
            qml/tst_qml_screens.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/boot/bootQmlLinkage.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../app/app.qrc)
        target_link_libraries(tst_qml_screens PRIVATE appcore Qt6::Qml Qt6::Quick Qt6::Test)
        target_compile_definitions(tst_qml_screens PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
        add_test(NAME tst_qml_screens COMMAND tst_qml_screens)
        set_tests_properties(tst_qml_screens PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS tst_qml_screens)
    endif()
endif()

# --- Comprehensive GUI tests (real app shell, headless offscreen) ----------------
# These boot the REAL application UI (App.qml + AppWindow + the live C++ Router
# driving both StackViews) through a QQuickView and drive navigation / edits /
# synthesized input, asserting persistence (save -> reopen), cross-file independence,
# byte-fidelity, and ZERO QML warnings. Like tst_qml_screens each compiles in app.qrc
# (qrc:/ui/app/... + the Material-style qtquickcontrols2.conf) and the exe's own
# bootQmlLinkage.cpp (one shared source of truth for QML type registration), links
# appcore, and runs on the offscreen platform so it gates BOTH the Linux and Windows
# CI jobs. The shared harness is tests/helpers/guiapp.h.
if(TARGET appcore)
    function(pse_add_gui_test name)
        qt_add_executable(${name}
            ${ARGN}
            ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/boot/bootQmlLinkage.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../app/app.qrc)
        target_link_libraries(${name} PRIVATE appcore Qt6::Qml Qt6::Quick Qt6::Test)
        target_compile_definitions(${name} PRIVATE PSE_ASSETS_DIR="${PSE_ASSETS_DIR}")
        add_test(NAME ${name} COMMAND ${name})
        set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        set_property(GLOBAL APPEND PROPERTY PSE_TEST_TARGETS ${name})
    endfunction()

    pse_add_gui_test(tst_gui_navigation gui/tst_gui_navigation.cpp)
    pse_add_gui_test(tst_gui_saveload   gui/tst_gui_saveload.cpp)
    pse_add_gui_test(tst_gui_input      gui/tst_gui_input.cpp)
    pse_add_gui_test(tst_gui_fidelity   gui/tst_gui_fidelity.cpp)
    pse_add_gui_test(tst_gui_drag       gui/tst_gui_drag.cpp)
    pse_add_gui_test(tst_gui_moves      gui/tst_gui_moves.cpp)
    pse_add_gui_test(tst_visual_regression gui/tst_visual_regression.cpp)
    pse_add_gui_test(tst_acceptance     gui/tst_acceptance.cpp)

    # Screenshot / animation-frame capture tool (NOT a CTest test). Same wiring as a
    # GUI test (appcore + bootQmlLinkage + app.qrc, runs offscreen), but it renders
    # the live UI to PNGs via grabWindow() instead of asserting. Build + run manually
    # (or via scripts/capture_screenshots.*) -- output goes to <repo>/tmp/screenshots
    # (git-ignored). See notes/reference/screenshots.md.
    get_filename_component(PSE_SCREENSHOTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../tmp/screenshots" ABSOLUTE)
    qt_add_executable(screenshooter
        tools/screenshooter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/boot/bootQmlLinkage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/app.qrc)
    target_link_libraries(screenshooter PRIVATE appcore Qt6::Qml Qt6::Quick Qt6::Test)
    target_compile_definitions(screenshooter PRIVATE
        PSE_ASSETS_DIR="${PSE_ASSETS_DIR}"
        PSE_SCREENSHOTS_DIR="${PSE_SCREENSHOTS_DIR}")
endif()

# Aggregate target: builds every registered test exe in one shot. CI builds this
# (`cmake --build build --target tests_all`) instead of an explicit, drift-prone
# list -- whatever ctest knows about, this guarantees is built.
get_property(_pse_test_targets GLOBAL PROPERTY PSE_TEST_TARGETS)
add_custom_target(tests_all)
if(_pse_test_targets)
    add_dependencies(tests_all ${_pse_test_targets})
endif()


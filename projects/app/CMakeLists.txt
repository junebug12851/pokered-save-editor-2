# The app's testable logic (bridge, MVC models, engine) lives in a library
# `appcore` so it can be linked by BOTH the executable and the test suite. The
# executable itself is just main/boot/mainwindow/QML-resources on top of appcore.
#
# Default: STATIC (avoids needing dllexport macros on every class; matches the
# shipped build). For the COVERAGE build, pass -DPSE_SHARED_APPCORE=ON: a STATIC
# appcore is embedded into every test exe, which defeats llvm-cov (it can't merge
# coverage across many binaries that each statically link identical TUs -- it
# reports executed files as 0%). A SHARED appcore is instrumented once and all
# test exes share it, so coverage merges cleanly. WINDOWS_EXPORT_ALL_SYMBOLS lets
# the shared build work without adding export macros to every class.
option(PSE_SHARED_APPCORE "Build appcore as a shared lib (for clean coverage measurement)" OFF)
if(PSE_SHARED_APPCORE)
    set(PSE_APPCORE_TYPE SHARED)
else()
    set(PSE_APPCORE_TYPE STATIC)
endif()

qt_add_library(appcore ${PSE_APPCORE_TYPE}
    src/bridge/bridge.cpp
    src/bridge/bridge.h
    src/bridge/router.cpp
    src/bridge/router.h
    src/bridge/settings.cpp
    src/bridge/settings.h
    src/engine/fontpreviewprovider.cpp
    src/engine/fontpreviewprovider.h
    src/engine/mapengine.cpp
    src/engine/mapengine.h
    src/engine/mapprovider.cpp
    src/engine/mapprovider.h
    src/engine/playerprovider.cpp
    src/engine/playerprovider.h
    src/engine/tilesetengine.cpp
    src/engine/tilesetengine.h
    src/engine/tilesetprovider.cpp
    src/engine/tilesetprovider.h
    src/mvc/creditsmodel.cpp
    src/mvc/creditsmodel.h
    src/mvc/fontkeyboard.cpp
    src/mvc/fontkeyboard.h
    src/mvc/mapmodel.cpp
    src/mvc/mapmodel.h
    src/mvc/itemmarketmodel.cpp
    src/mvc/itemmarketmodel.h
    src/mvc/itemmarketcartmodel.cpp
    src/mvc/itemmarketcartmodel.h
    src/mvc/itemmarketviewmodel.cpp
    src/mvc/itemmarketviewmodel.h
    src/mvc/itemmarket/itemmarketentry.cpp
    src/mvc/itemmarket/itemmarketentry.h
    src/mvc/itemmarket/itemmarketentrygcpokemon.cpp
    src/mvc/itemmarket/itemmarketentrygcpokemon.h
    src/mvc/itemmarket/itemmarketentrymessage.cpp
    src/mvc/itemmarket/itemmarketentrymessage.h
    src/mvc/itemmarket/itemmarketentrymoney.cpp
    src/mvc/itemmarket/itemmarketentrymoney.h
    src/mvc/itemmarket/itemmarketentryplayeritem.cpp
    src/mvc/itemmarket/itemmarketentryplayeritem.h
    src/mvc/itemmarket/itemmarketentrystoreitem.cpp
    src/mvc/itemmarket/itemmarketentrystoreitem.h
    src/mvc/itemselectmodel.cpp
    src/mvc/itemselectmodel.h
    src/mvc/itemstoragemodel.cpp
    src/mvc/itemstoragemodel.h
    src/mvc/itemoverviewmodel.cpp
    src/mvc/itemoverviewmodel.h
    src/mvc/itemexchangemodel.cpp
    src/mvc/itemexchangemodel.h
    src/mvc/pokemonoverviewmodel.cpp
    src/mvc/pokemonoverviewmodel.h
    src/mvc/moveselectmodel.cpp
    src/mvc/moveselectmodel.h
    src/mvc/natureselectmodel.cpp
    src/mvc/natureselectmodel.h
    src/mvc/pokedexmodel.cpp
    src/mvc/pokedexmodel.h
    src/mvc/pokemonboxselectmodel.cpp
    src/mvc/pokemonboxselectmodel.h
    src/mvc/pokemonstartersmodel.cpp
    src/mvc/pokemonstartersmodel.h
    src/mvc/pokemonstoragemodel.cpp
    src/mvc/pokemonstoragemodel.h
    src/mvc/recentfilesmodel.cpp
    src/mvc/recentfilesmodel.h
    src/mvc/speciesselectmodel.cpp
    src/mvc/speciesselectmodel.h
    src/mvc/statusselectmodel.cpp
    src/mvc/statusselectmodel.h
    src/mvc/typesmodel.cpp
    src/mvc/typesmodel.h
)

target_include_directories(appcore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# When shared, auto-export all symbols so test exes link without per-class export
# macros (no-op for the default static build).
if(PSE_SHARED_APPCORE)
    set_target_properties(appcore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_link_libraries(appcore
    PUBLIC
        common
        db
        savefile
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickWidgets
        Qt6::Svg
        Qt6::Qml
        Qt6::Charts
)

qt_add_executable(PokeredSaveEditor
    src/main.cpp
    src/boot/boot.cpp
    src/boot/bootDatabase.cpp
    src/boot/bootQmlLinkage.cpp
    src/boot/debuglaunch.cpp
    src/boot/debugserver.cpp
    ui/window/mainwindow.cpp
    ui/window/mainwindow.h
    ui/window/mainwindow.ui
    app.qrc
)

target_include_directories(PokeredSaveEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

target_link_libraries(PokeredSaveEditor
    PRIVATE
        appcore
        Qt6::Network
)

# DEBUG hot-reload (--hot): let the app find its QML source tree on disk.
target_compile_definitions(PokeredSaveEditor PRIVATE PSE_QML_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# --- App version (from VERSION via the generated pse_version.h) --------------
# boot.cpp includes "pse_version.h" to set the runtime version. The header is
# produced by the top-level pse_version_gen target into ${PSE_VERSION_HEADER_DIR}.
add_dependencies(PokeredSaveEditor pse_version_gen)
target_include_directories(PokeredSaveEditor PRIVATE ${PSE_VERSION_HEADER_DIR})

# Windows: compile the generated VERSIONINFO resource into the .exe so its
# Properties -> Details shows the version. Guarded by PSE_GEN_WINRC (top level).
if(WIN32 AND PSE_GEN_WINRC)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/pse_app.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/pse_app.rc @ONLY)
    target_sources(PokeredSaveEditor PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/pse_app.rc)
endif()

# --- Internationalization (i18n) — Qt Linguist pipeline ----------------------
# Source language is en_US (the literal strings in the code). Each shipped locale
# gets a translations/pse_<locale>.ts catalog; lupdate extracts qsTr()/tr() calls
# into it, lrelease compiles it to a binary .qm, and qt_add_translations embeds
# the .qm under the ":/i18n" resource prefix. boot.cpp installs a QTranslator that
# loads the right .qm at startup; English (or any untranslated string) just shows
# the source text — graceful degradation, no missing-string blanks.
#
# To add a language: add translations/pse_<locale>.ts below, build the
# `update_translations` target once to populate it, translate it in Qt Linguist,
# and rebuild. Full guide: notes/reference/i18n.md.
#
# Translatable sources. QML lives inside app.qrc (not target SOURCES), so it is
# gathered explicitly for lupdate; C++ (appcore models + boot/bridge) is scanned
# too so tr() in models is picked up. CONFIGURE_DEPENDS re-globs on each build.
set(PSE_TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/pse_en.ts
)

file(GLOB_RECURSE PSE_I18N_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/*.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

qt_add_translations(PokeredSaveEditor
    TS_FILES ${PSE_TS_FILES}
    SOURCES ${PSE_I18N_SOURCES}
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS -no-obsolete -locations none
)

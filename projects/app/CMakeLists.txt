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
    src/engine/tilesetengine.cpp
    src/engine/tilesetengine.h
    src/engine/tilesetprovider.cpp
    src/engine/tilesetprovider.h
    src/mvc/creditsmodel.cpp
    src/mvc/creditsmodel.h
    src/mvc/fontsearchmodel.cpp
    src/mvc/fontsearchmodel.h
    src/mvc/individualmap.cpp
    src/mvc/individualmap.h
    src/mvc/itemmarketmodel.cpp
    src/mvc/itemmarketmodel.h
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
    src/mvc/pokemonoverviewmodel.cpp
    src/mvc/pokemonoverviewmodel.h
    src/mvc/mapselectmodel.cpp
    src/mvc/mapselectmodel.h
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
)

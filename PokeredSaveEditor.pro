QT += quick quickcontrols2 core widgets quickwidgets svg
CONFIG += c++1z

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

FORMS += \
  ui/window/mainwindow.ui

HEADERS += \
  src/bridge/bridge.h \
  src/bridge/router.h \
  src/common/types.h \
  src/common/utility.h \
  src/data/db/credits.h \
  src/data/db/eventpokemon.h \
  src/data/db/events.h \
  src/data/db/examplesplayer.h \
  src/data/db/examplespokemon.h \
  src/data/db/examplesrival.h \
  src/data/db/fly.h \
  src/data/db/fonts.h \
  src/data/db/fontsearch.h \
  src/data/db/gamecorner.h \
  src/data/db/gamedata.h \
  src/data/db/hiddenCoins.h \
  src/data/db/hiddenItems.h \
  src/data/db/items.h \
  src/data/db/maps.h \
  src/data/db/mapsearch.h \
  src/data/db/missables.h \
  src/data/db/moves.h \
  src/data/db/music.h \
  src/data/db/names.h \
  src/data/db/namesPokemon.h \
  src/data/db/pokemon.h \
  src/data/db/scripts.h \
  src/data/db/spriteSet.h \
  src/data/db/sprites.h \
  src/data/db/starterPokemon.h \
  src/data/db/tileset.h \
  src/data/db/tmHm.h \
  src/data/db/trades.h \
  src/data/db/trainers.h \
  src/data/db/types.h \
  src/data/file/expanded/area/area.h \
  src/data/file/expanded/area/areaaudio.h \
  src/data/file/expanded/area/areageneral.h \
  src/data/file/expanded/area/arealoadedsprites.h \
  src/data/file/expanded/area/areamap.h \
  src/data/file/expanded/area/areanpc.h \
  src/data/file/expanded/area/areaplayer.h \
  src/data/file/expanded/area/areapokemon.h \
  src/data/file/expanded/area/areapuzzle.h \
  src/data/file/expanded/area/areasign.h \
  src/data/file/expanded/area/areasprites.h \
  src/data/file/expanded/area/areatileset.h \
  src/data/file/expanded/area/areawarps.h \
  src/data/file/expanded/daycare.h \
  src/data/file/expanded/fragments/hofpokemon.h \
  src/data/file/expanded/fragments/hofrecord.h \
  src/data/file/expanded/fragments/item.h \
  src/data/file/expanded/fragments/itemstoragebox.h \
  src/data/file/expanded/fragments/mapconndata.h \
  src/data/file/expanded/fragments/pokemonbox.h \
  src/data/file/expanded/fragments/pokemonparty.h \
  src/data/file/expanded/fragments/pokemonstoragebox.h \
  src/data/file/expanded/fragments/pokemonstorageset.h \
  src/data/file/expanded/fragments/signdata.h \
  src/data/file/expanded/fragments/spritedata.h \
  src/data/file/expanded/fragments/warpdata.h \
  src/data/file/expanded/halloffame.h \
  src/data/file/expanded/player/player.h \
  src/data/file/expanded/player/playerbasics.h \
  src/data/file/expanded/player/playerpokedex.h \
  src/data/file/expanded/player/playerpokemon.h \
  src/data/file/expanded/rival.h \
  src/data/file/expanded/savefileexpanded.h \
  src/data/file/expanded/storage.h \
  src/data/file/expanded/world/world.h \
  src/data/file/expanded/world/worldcompleted.h \
  src/data/file/expanded/world/worldevents.h \
  src/data/file/expanded/world/worldgeneral.h \
  src/data/file/expanded/world/worldhidden.h \
  src/data/file/expanded/world/worldmissables.h \
  src/data/file/expanded/world/worldother.h \
  src/data/file/expanded/world/worldscripts.h \
  src/data/file/expanded/world/worldtowns.h \
  src/data/file/expanded/world/worldtrades.h \
  src/data/file/filemanagement.h \
  src/data/file/savefile.h \
  src/data/file/savefileiterator.h \
  src/data/file/savefiletoolset.h \
  src/bridge/settings.h \
  src/engine/fontpreviewprovider.h \
  src/engine/tilesetprovider.h \
  src/mvc/itemmarket/itemmarketentry.h \
  src/mvc/itemmarket/itemmarketentrygcpokemon.h \
  src/mvc/itemmarket/itemmarketentrymessage.h \
  src/mvc/itemmarket/itemmarketentryplayeritem.h \
  src/mvc/itemmarket/itemmarketentrystoreitem.h \
  src/mvc/itemmarketmodel.h \
  src/mvc/itemselectmodel.h \
  src/mvc/itemstoragemodel.h \
  src/mvc/creditsmodel.h \
  src/common/random.h \
  src/mvc/fontsearchmodel.h \
  src/mvc/pokedexmodel.h \
  src/mvc/pokemonstartersmodel.h \
  src/mvc/recentfilesmodel.h \
  src/engine/tilesetengine.h \
  ui/window/mainwindow.h

SOURCES += \
  src/boot/bootQmlLinkage.cpp \
  src/boot/boot.cpp \
  src/boot/bootDatabase.cpp \
  src/bridge/bridge.cpp \
  src/bridge/router.cpp \
  src/common/utility.cpp \
  src/data/db/credits.cpp \
  src/data/db/examplesplayer.cpp \
  src/data/db/examplespokemon.cpp \
  src/data/db/examplesrival.cpp \
  src/data/db/fontsearch.cpp \
  src/data/db/gamecorner.cpp \
  src/data/db/mapsearch.cpp \
  src/data/db/music.cpp \
  src/data/db/namesPokemon.cpp \
  src/data/db/spriteSet.cpp \
  src/data/db/tileset.cpp \
  src/data/file/expanded/area/area.cpp \
  src/data/file/expanded/area/areaaudio.cpp \
  src/data/file/expanded/area/areageneral.cpp \
  src/data/file/expanded/area/arealoadedsprites.cpp \
  src/data/file/expanded/area/areamap.cpp \
  src/data/file/expanded/area/areanpc.cpp \
  src/data/file/expanded/area/areaplayer.cpp \
  src/data/file/expanded/area/areapokemon.cpp \
  src/data/file/expanded/area/areapuzzle.cpp \
  src/data/file/expanded/area/areasign.cpp \
  src/data/file/expanded/area/areasprites.cpp \
  src/data/file/expanded/area/areatileset.cpp \
  src/data/file/expanded/area/areawarps.cpp \
  src/data/file/expanded/daycare.cpp \
  src/data/file/expanded/fragments/hofpokemon.cpp \
  src/data/file/expanded/fragments/hofrecord.cpp \
  src/data/file/expanded/fragments/item.cpp \
  src/data/file/expanded/fragments/itemstoragebox.cpp \
  src/data/file/expanded/fragments/mapconndata.cpp \
  src/data/file/expanded/fragments/pokemonbox.cpp \
  src/data/file/expanded/fragments/pokemonparty.cpp \
  src/data/file/expanded/fragments/pokemonstoragebox.cpp \
  src/data/file/expanded/fragments/pokemonstorageset.cpp \
  src/data/file/expanded/fragments/signdata.cpp \
  src/data/file/expanded/fragments/spritedata.cpp \
  src/data/file/expanded/fragments/warpdata.cpp \
  src/data/file/expanded/halloffame.cpp \
  src/data/file/expanded/player/player.cpp \
  src/data/file/expanded/player/playerbasics.cpp \
  src/data/file/expanded/player/playerpokedex.cpp \
  src/data/file/expanded/player/playerpokemon.cpp \
  src/data/file/expanded/rival.cpp \
  src/data/file/expanded/savefileexpanded.cpp \
  src/data/file/expanded/storage.cpp \
  src/data/file/expanded/world/world.cpp \
  src/data/file/expanded/world/worldcompleted.cpp \
  src/data/file/expanded/world/worldevents.cpp \
  src/data/file/expanded/world/worldgeneral.cpp \
  src/data/file/expanded/world/worldhidden.cpp \
  src/data/file/expanded/world/worldmissables.cpp \
  src/data/file/expanded/world/worldother.cpp \
  src/data/file/expanded/world/worldscripts.cpp \
  src/data/file/expanded/world/worldtowns.cpp \
  src/data/file/expanded/world/worldtrades.cpp \
  src/data/file/filemanagement.cpp \
  src/data/file/savefile.cpp \
  src/data/file/savefileiterator.cpp \
  src/data/file/savefiletoolset.cpp \
  src/bridge/settings.cpp \
  src/engine/fontpreviewprovider.cpp \
  src/engine/tilesetprovider.cpp \
  src/main.cpp \
  src/data/db/eventpokemon.cpp \
  src/data/db/events.cpp \
  src/data/db/fly.cpp \
  src/data/db/fonts.cpp \
  src/data/db/gamedata.cpp \
  src/data/db/hiddenCoins.cpp \
  src/data/db/hiddenItems.cpp \
  src/data/db/items.cpp \
  src/data/db/maps.cpp \
  src/data/db/missables.cpp \
  src/data/db/moves.cpp \
  src/data/db/names.cpp \
  src/data/db/pokemon.cpp \
  src/data/db/scripts.cpp \
  src/data/db/sprites.cpp \
  src/data/db/starterPokemon.cpp \
  src/data/db/tmHm.cpp \
  src/data/db/trades.cpp \
  src/data/db/trainers.cpp \
  src/data/db/types.cpp \
  src/mvc/itemmarket/itemmarketentry.cpp \
  src/mvc/itemmarket/itemmarketentrygcpokemon.cpp \
  src/mvc/itemmarket/itemmarketentrymessage.cpp \
  src/mvc/itemmarket/itemmarketentryplayeritem.cpp \
  src/mvc/itemmarket/itemmarketentrystoreitem.cpp \
  src/mvc/itemmarketmodel.cpp \
  src/mvc/itemselectmodel.cpp \
  src/mvc/itemstoragemodel.cpp \
  src/mvc/creditsmodel.cpp \
  src/common/random.cpp \
  src/mvc/fontsearchmodel.cpp \
  src/mvc/pokedexmodel.cpp \
  src/mvc/pokemonstartersmodel.cpp \
  src/mvc/recentfilesmodel.cpp \
  src/engine/tilesetengine.cpp \
  ui/window/mainwindow.cpp

RESOURCES += qml.qrc

RC_ICONS = assets/icons/app/icon.ico

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

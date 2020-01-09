QT += quick quickcontrols2 core widgets quickwidgets
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
  src/common/types.h \
  src/data/db/eventpokemon.h \
  src/data/db/events.h \
  src/data/db/fly.h \
  src/data/db/fonts.h \
  src/data/db/fontsearch.h \
  src/data/db/gamedata.h \
  src/data/db/hiddenCoins.h \
  src/data/db/hiddenItems.h \
  src/data/db/items.h \
  src/data/db/maps.h \
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
  src/data/file/expanded/expandedinterface.h \
  src/data/file/expanded/fragments/hofpokemon.h \
  src/data/file/expanded/fragments/hofrecord.h \
  src/data/file/expanded/fragments/mapconndata.h \
  src/data/file/expanded/fragments/pokemonbox.h \
  src/data/file/expanded/fragments/pokemonparty.h \
  src/data/file/expanded/fragments/signdata.h \
  src/data/file/expanded/fragments/spritedata.h \
  src/data/file/expanded/fragments/warpdata.h \
  src/data/file/expanded/player/player.h \
  src/data/file/expanded/player/playerbasics.h \
  src/data/file/expanded/player/playeritems.h \
  src/data/file/expanded/player/playerpokedex.h \
  src/data/file/expanded/player/playerpokemon.h \
  src/data/file/expanded/savefileexpanded.h \
  src/data/file/filemanagement.h \
  src/data/file/savefile.h \
  src/data/file/savefileiterator.h \
  src/data/file/savefiletoolset.h \
  ui/window/mainwindow.h

SOURCES += \
  src/boot.cpp \
  src/data/db/fontsearch.cpp \
  src/data/db/music.cpp \
  src/data/db/namesPokemon.cpp \
  src/data/db/spriteSet.cpp \
  src/data/db/tileset.cpp \
  src/data/file/expanded/area/area.cpp \
  src/data/file/expanded/area/areaaudio.cpp \
  src/data/file/expanded/area/areageneral.cpp \
  src/data/file/expanded/area/arealoadedsprites.cpp \
  src/data/file/expanded/fragments/hofpokemon.cpp \
  src/data/file/expanded/fragments/hofrecord.cpp \
  src/data/file/expanded/fragments/mapconndata.cpp \
  src/data/file/expanded/fragments/pokemonbox.cpp \
  src/data/file/expanded/fragments/pokemonparty.cpp \
  src/data/file/expanded/fragments/signdata.cpp \
  src/data/file/expanded/fragments/spritedata.cpp \
  src/data/file/expanded/fragments/warpdata.cpp \
  src/data/file/expanded/player/player.cpp \
  src/data/file/expanded/player/playerbasics.cpp \
  src/data/file/expanded/player/playeritems.cpp \
  src/data/file/expanded/player/playerpokedex.cpp \
  src/data/file/expanded/player/playerpokemon.cpp \
  src/data/file/expanded/savefileexpanded.cpp \
  src/data/file/filemanagement.cpp \
  src/data/file/savefile.cpp \
  src/data/file/savefileiterator.cpp \
  src/data/file/savefiletoolset.cpp \
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
  ui/window/mainwindow.cpp

RESOURCES += qml.qrc

RC_ICONS = assets/icons/icon.ico

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

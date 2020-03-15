QT = core qml

TEMPLATE = lib
DEFINES += DB_LIBRARY

CONFIG += c++1z

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/pse-db/creditsdb.cpp \
    src/pse-db/entries/eventpokemondbentry.cpp \
    src/pse-db/entries\creditdbentry.cpp \
    src/pse-db/db.cpp \
    src/pse-db/eventpokemondb.cpp \
    src/pse-db/events.cpp \
    src/pse-db/examplesplayer.cpp \
    src/pse-db/examplespokemon.cpp \
    src/pse-db/examplesrival.cpp \
    src/pse-db/fly.cpp \
    src/pse-db/fonts.cpp \
    src/pse-db/util/fontsearch.cpp \
    src/pse-db/gamecorner.cpp \
    src/pse-db/util/gamedata.cpp \
    src/pse-db/hiddenCoins.cpp \
    src/pse-db/hiddenItems.cpp \
    src/pse-db/items.cpp \
    src/pse-db/maps.cpp \
    src/pse-db/util/mapsearch.cpp \
    src/pse-db/missables.cpp \
    src/pse-db/moves.cpp \
    src/pse-db/music.cpp \
    src/pse-db/names.cpp \
    src/pse-db/namesPokemon.cpp \
    src/pse-db/pokemon.cpp \
    src/pse-db/scripts.cpp \
    src/pse-db/spriteSet.cpp \
    src/pse-db/sprites.cpp \
    src/pse-db/starterPokemon.cpp \
    src/pse-db/tileset.cpp \
    src/pse-db/tmHm.cpp \
    src/pse-db/trades.cpp \
    src/pse-db/trainers.cpp \
    src/pse-db/types.cpp

HEADERS += \
    src/pse-db/creditsdb.h \
    src/pse-db/entries/eventpokemondbentry.h \
    src/pse-db/entries\creditdbentry.h \
    src/pse-db/db.h \
    src/pse-db/db_autoport.h \
    src/pse-db/eventpokemondb.h \
    src/pse-db/events.h \
    src/pse-db/examplesplayer.h \
    src/pse-db/examplespokemon.h \
    src/pse-db/examplesrival.h \
    src/pse-db/fly.h \
    src/pse-db/fonts.h \
    src/pse-db/util/fontsearch.h \
    src/pse-db/gamecorner.h \
    src/pse-db/util/gamedata.h \
    src/pse-db/hiddenCoins.h \
    src/pse-db/hiddenItems.h \
    src/pse-db/items.h \
    src/pse-db/maps.h \
    src/pse-db/util/mapsearch.h \
    src/pse-db/missables.h \
    src/pse-db/moves.h \
    src/pse-db/music.h \
    src/pse-db/names.h \
    src/pse-db/namesPokemon.h \
    src/pse-db/pokemon.h \
    src/pse-db/scripts.h \
    src/pse-db/spriteSet.h \
    src/pse-db/sprites.h \
    src/pse-db/starterPokemon.h \
    src/pse-db/tileset.h \
    src/pse-db/tmHm.h \
    src/pse-db/trades.h \
    src/pse-db/trainers.h \
    src/pse-db/types.h

RESOURCES += db.qrc

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

LIBS += \
    -L$$OUT_PWD/../common/ -lcommon

INCLUDEPATH += \
    $$PWD/../common/src

DEPENDPATH += \
    $$PWD/../common/src

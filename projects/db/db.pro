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
    src/pse-db/abstracthiddenitemdb.cpp \
    src/pse-db/creditsdb.cpp \
    src/pse-db/entries/abstractrandomstring.cpp \
    src/pse-db/entries/eventdbentry.cpp \
    src/pse-db/entries/eventpokemondbentry.cpp \
    src/pse-db/entries/flydbentry.cpp \
    src/pse-db/entries/fontdbentry.cpp \
    src/pse-db/entries/gamecornerdbentry.cpp \
    src/pse-db/entries/hiddenitemdbentry.cpp \
    src/pse-db/entries/itemdbentry.cpp \
    src/pse-db/entries/mapdbentry.cpp \
    src/pse-db/entries/mapdbentryconnect.cpp \
    src/pse-db/entries/mapdbentrysign.cpp \
    src/pse-db/entries/mapdbentrysprite.cpp \
    src/pse-db/entries/mapdbentryspriteitem.cpp \
    src/pse-db/entries/mapdbentryspritenpc.cpp \
    src/pse-db/entries/mapdbentryspritepokemon.cpp \
    src/pse-db/entries/mapdbentryspritetrainer.cpp \
    src/pse-db/entries/mapdbentrywarpin.cpp \
    src/pse-db/entries/mapdbentrywarpout.cpp \
    src/pse-db/entries/mapdbentrywildmon.cpp \
    src/pse-db/entries/missabledbentry.cpp \
    src/pse-db/entries\creditdbentry.cpp \
    src/pse-db/db.cpp \
    src/pse-db/eventpokemondb.cpp \
    src/pse-db/eventsdb.cpp \
    src/pse-db/entries/examplesplayer.cpp \
    src/pse-db/entries/examplespokemon.cpp \
    src/pse-db/entries/examplesrival.cpp \
    src/pse-db/examples.cpp \
    src/pse-db/flydb.cpp \
    src/pse-db/fontsdb.cpp \
    src/pse-db/gamecornerdb.cpp \
    src/pse-db/hiddenItemsdb.cpp \
    src/pse-db/hiddencoinsdb.cpp \
    src/pse-db/itemsdb.cpp \
    src/pse-db/mapsdb.cpp \
    src/pse-db/missablesdb.cpp \
    src/pse-db/names.cpp \
    src/pse-db/util/fontsearch.cpp \
    src/pse-db/util/gamedata.cpp \
    src/pse-db/util/hiddencoinsdb.cpp \
    src/pse-db/util/mapsearch.cpp \
    src/pse-db/moves.cpp \
    src/pse-db/music.cpp \
    src/pse-db/entries/namesplayer.cpp \
    src/pse-db/entries/namespokemon.cpp \
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
    src/pse-db/abstracthiddenitemdb.h \
    src/pse-db/creditsdb.h \
    src/pse-db/entries/abstractrandomstring.h \
    src/pse-db/entries/eventdbentry.h \
    src/pse-db/entries/eventpokemondbentry.h \
    src/pse-db/entries/flydbentry.h \
    src/pse-db/entries/fontdbentry.h \
    src/pse-db/entries/gamecornerdbentry.h \
    src/pse-db/entries/hiddenitemdbentry.h \
    src/pse-db/entries/itemdbentry.h \
    src/pse-db/entries/mapdbentry.h \
    src/pse-db/entries/mapdbentryconnect.h \
    src/pse-db/entries/mapdbentrysign.h \
    src/pse-db/entries/mapdbentrysprite.h \
    src/pse-db/entries/mapdbentryspriteitem.h \
    src/pse-db/entries/mapdbentryspritenpc.h \
    src/pse-db/entries/mapdbentryspritepokemon.h \
    src/pse-db/entries/mapdbentryspritetrainer.h \
    src/pse-db/entries/mapdbentrywarpin.h \
    src/pse-db/entries/mapdbentrywarpout.h \
    src/pse-db/entries/mapdbentrywildmon.h \
    src/pse-db/entries/missabledbentry.h \
    src/pse-db/entries\creditdbentry.h \
    src/pse-db/db.h \
    src/pse-db/db_autoport.h \
    src/pse-db/eventpokemondb.h \
    src/pse-db/eventsdb.h \
    src/pse-db/entries/examplesplayer.h \
    src/pse-db/entries/examplespokemon.h \
    src/pse-db/entries/examplesrival.h \
    src/pse-db/examples.h \
    src/pse-db/flydb.h \
    src/pse-db/fontsdb.h \
    src/pse-db/gamecornerdb.h \
    src/pse-db/hiddenItemsdb.h \
    src/pse-db/hiddencoinsdb.h \
    src/pse-db/itemsdb.h \
    src/pse-db/mapsdb.h \
    src/pse-db/missablesdb.h \
    src/pse-db/names.h \
    src/pse-db/util/fontsearch.h \
    src/pse-db/util/gamedata.h \
    src/pse-db/util/hiddencoinsdb.h \
    src/pse-db/util/mapsearch.h \
    src/pse-db/moves.h \
    src/pse-db/music.h \
    src/pse-db/entries/namesplayer.h \
    src/pse-db/entries/namespokemon.h \
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

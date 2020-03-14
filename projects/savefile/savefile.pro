QT += quick quickcontrols2 core widgets quickwidgets svg

TEMPLATE = lib
DEFINES += SAVEFILE_LIBRARY

CONFIG += c++1z

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/pse-savefile/expanded/area/area.cpp \
    src/pse-savefile/expanded/area/areaaudio.cpp \
    src/pse-savefile/expanded/area/areageneral.cpp \
    src/pse-savefile/expanded/area/arealoadedsprites.cpp \
    src/pse-savefile/expanded/area/areamap.cpp \
    src/pse-savefile/expanded/area/areanpc.cpp \
    src/pse-savefile/expanded/area/areaplayer.cpp \
    src/pse-savefile/expanded/area/areapokemon.cpp \
    src/pse-savefile/expanded/area/areasign.cpp \
    src/pse-savefile/expanded/area/areasprites.cpp \
    src/pse-savefile/expanded/area/areatileset.cpp \
    src/pse-savefile/expanded/area/areawarps.cpp \
    src/pse-savefile/expanded/daycare.cpp \
    src/pse-savefile/expanded/fragments/hofpokemon.cpp \
    src/pse-savefile/expanded/fragments/hofrecord.cpp \
    src/pse-savefile/expanded/fragments/item.cpp \
    src/pse-savefile/expanded/fragments/itemstoragebox.cpp \
    src/pse-savefile/expanded/fragments/mapconndata.cpp \
    src/pse-savefile/expanded/fragments/pokemonbox.cpp \
    src/pse-savefile/expanded/fragments/pokemonparty.cpp \
    src/pse-savefile/expanded/fragments/pokemonstoragebox.cpp \
    src/pse-savefile/expanded/fragments/pokemonstorageset.cpp \
    src/pse-savefile/expanded/fragments/signdata.cpp \
    src/pse-savefile/expanded/fragments/spritedata.cpp \
    src/pse-savefile/expanded/fragments/warpdata.cpp \
    src/pse-savefile/expanded/halloffame.cpp \
    src/pse-savefile/expanded/player/player.cpp \
    src/pse-savefile/expanded/player/playerbasics.cpp \
    src/pse-savefile/expanded/player/playerpokedex.cpp \
    src/pse-savefile/expanded/player/playerpokemon.cpp \
    src/pse-savefile/expanded/rival.cpp \
    src/pse-savefile/expanded/savefileexpanded.cpp \
    src/pse-savefile/expanded/storage.cpp \
    src/pse-savefile/expanded/world/world.cpp \
    src/pse-savefile/expanded/world/worldcompleted.cpp \
    src/pse-savefile/expanded/world/worldevents.cpp \
    src/pse-savefile/expanded/world/worldgeneral.cpp \
    src/pse-savefile/expanded/world/worldhidden.cpp \
    src/pse-savefile/expanded/world/worldlocal.cpp \
    src/pse-savefile/expanded/world/worldmissables.cpp \
    src/pse-savefile/expanded/world/worldother.cpp \
    src/pse-savefile/expanded/world/worldscripts.cpp \
    src/pse-savefile/expanded/world/worldtowns.cpp \
    src/pse-savefile/expanded/world/worldtrades.cpp \
    src/pse-savefile/filemanagement.cpp \
    src/pse-savefile/savefile.cpp \
    src/pse-savefile/savefileiterator.cpp \
    src/pse-savefile/savefiletoolset.cpp

HEADERS += \
    src/pse-savefile/expanded/area/area.h \
    src/pse-savefile/expanded/area/areaaudio.h \
    src/pse-savefile/expanded/area/areageneral.h \
    src/pse-savefile/expanded/area/arealoadedsprites.h \
    src/pse-savefile/expanded/area/areamap.h \
    src/pse-savefile/expanded/area/areanpc.h \
    src/pse-savefile/expanded/area/areaplayer.h \
    src/pse-savefile/expanded/area/areapokemon.h \
    src/pse-savefile/expanded/area/areasign.h \
    src/pse-savefile/expanded/area/areasprites.h \
    src/pse-savefile/expanded/area/areatileset.h \
    src/pse-savefile/expanded/area/areawarps.h \
    src/pse-savefile/expanded/daycare.h \
    src/pse-savefile/expanded/fragments/hofpokemon.h \
    src/pse-savefile/expanded/fragments/hofrecord.h \
    src/pse-savefile/expanded/fragments/item.h \
    src/pse-savefile/expanded/fragments/itemstoragebox.h \
    src/pse-savefile/expanded/fragments/mapconndata.h \
    src/pse-savefile/expanded/fragments/pokemonbox.h \
    src/pse-savefile/expanded/fragments/pokemonparty.h \
    src/pse-savefile/expanded/fragments/pokemonstoragebox.h \
    src/pse-savefile/expanded/fragments/pokemonstorageset.h \
    src/pse-savefile/expanded/fragments/signdata.h \
    src/pse-savefile/expanded/fragments/spritedata.h \
    src/pse-savefile/expanded/fragments/warpdata.h \
    src/pse-savefile/expanded/halloffame.h \
    src/pse-savefile/expanded/player/player.h \
    src/pse-savefile/expanded/player/playerbasics.h \
    src/pse-savefile/expanded/player/playerpokedex.h \
    src/pse-savefile/expanded/player/playerpokemon.h \
    src/pse-savefile/expanded/rival.h \
    src/pse-savefile/expanded/savefileexpanded.h \
    src/pse-savefile/expanded/storage.h \
    src/pse-savefile/expanded/world/world.h \
    src/pse-savefile/expanded/world/worldcompleted.h \
    src/pse-savefile/expanded/world/worldevents.h \
    src/pse-savefile/expanded/world/worldgeneral.h \
    src/pse-savefile/expanded/world/worldhidden.h \
    src/pse-savefile/expanded/world/worldlocal.h \
    src/pse-savefile/expanded/world/worldmissables.h \
    src/pse-savefile/expanded/world/worldother.h \
    src/pse-savefile/expanded/world/worldscripts.h \
    src/pse-savefile/expanded/world/worldtowns.h \
    src/pse-savefile/expanded/world/worldtrades.h \
    src/pse-savefile/filemanagement.h \
    src/pse-savefile/savefile.h \
    src/pse-savefile/savefile_autoport.h \
    src/pse-savefile/savefileiterator.h \
    src/pse-savefile/savefiletoolset.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

LIBS += \
    -L$$OUT_PWD/../common/ -lcommon \
    -L$$OUT_PWD/../db/ -ldb

INCLUDEPATH += \
    $$PWD/../common/src \
    $$PWD/../db/src

DEPENDPATH += \
    $$PWD/../common/src \
    $$PWD/../db/src

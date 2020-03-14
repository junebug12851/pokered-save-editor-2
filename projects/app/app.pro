QT += quick quickcontrols2 core widgets quickwidgets svg
CONFIG += c++1z

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

FORMS += \
  ui/window/mainwindow.ui

HEADERS += \
  src/bridge/bridge.h \
  src/bridge/router.h \
  src/bridge/settings.h \
  src/engine/fontpreviewprovider.h \
  src/engine/tilesetprovider.h \
  src/mvc/individualmap.h \
  src/mvc/itemmarket/itemmarketentry.h \
  src/mvc/itemmarket/itemmarketentrygcpokemon.h \
  src/mvc/itemmarket/itemmarketentrymessage.h \
  src/mvc/itemmarket/itemmarketentrymoney.h \
  src/mvc/itemmarket/itemmarketentryplayeritem.h \
  src/mvc/itemmarket/itemmarketentrystoreitem.h \
  src/mvc/itemmarketmodel.h \
  src/mvc/itemselectmodel.h \
  src/mvc/itemstoragemodel.h \
  src/mvc/pokemonstoragemodel.h \
  src/mvc/creditsmodel.h \
  src/mvc/fontsearchmodel.h \
  src/mvc/pokedexmodel.h \
  src/mvc/pokemonstartersmodel.h \
  src/mvc/pokemonboxselectmodel.h \
  src/mvc/recentfilesmodel.h \
  src/mvc/typesmodel.h \
  src/mvc/speciesselectmodel.h \
  src/mvc/statusselectmodel.h \
  src/mvc/natureselectmodel.h \
  src/mvc/moveselectmodel.h \
  src/mvc/mapselectmodel.h \
  src/engine/tilesetengine.h \
  ui/window/mainwindow.h

SOURCES += \
  src/boot/bootQmlLinkage.cpp \
  src/boot/boot.cpp \
  src/boot/bootDatabase.cpp \
  src/bridge/bridge.cpp \
  src/bridge/router.cpp \
  src/bridge/settings.cpp \
  src/engine/fontpreviewprovider.cpp \
  src/engine/tilesetprovider.cpp \
  src/main.cpp \
  src/mvc/individualmap.cpp \
  src/mvc/itemmarket/itemmarketentry.cpp \
  src/mvc/itemmarket/itemmarketentrygcpokemon.cpp \
  src/mvc/itemmarket/itemmarketentrymessage.cpp \
  src/mvc/itemmarket/itemmarketentrymoney.cpp \
  src/mvc/itemmarket/itemmarketentryplayeritem.cpp \
  src/mvc/itemmarket/itemmarketentrystoreitem.cpp \
  src/mvc/itemmarketmodel.cpp \
  src/mvc/itemselectmodel.cpp \
  src/mvc/itemstoragemodel.cpp \
  src/mvc/pokemonstoragemodel.cpp \
  src/mvc/creditsmodel.cpp \
  src/mvc/fontsearchmodel.cpp \
  src/mvc/pokedexmodel.cpp \
  src/mvc/pokemonstartersmodel.cpp \
  src/mvc/pokemonboxselectmodel.cpp \
  src/mvc/recentfilesmodel.cpp \
  src/mvc/typesmodel.cpp \
  src/mvc/speciesselectmodel.cpp \
  src/mvc/statusselectmodel.cpp \
  src/mvc/natureselectmodel.cpp \
  src/mvc/moveselectmodel.cpp \
  src/mvc/mapselectmodel.cpp \
  src/engine/tilesetengine.cpp \
  ui/window/mainwindow.cpp

RESOURCES += app.qrc

RC_ICONS = assets/icons/app/icon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += \
    -L$$OUT_PWD/../common/ -lcommon \
    -L$$OUT_PWD/../db/ -ldb \
    -L$$OUT_PWD/../savefile/ -lsavefile

INCLUDEPATH += \
    $$PWD/../common/src \
    $$PWD/../db/src \
    $$PWD/../savefile/src

DEPENDPATH += \
    $$PWD/../common/src \
    $$PWD/../db/src \
    $$PWD/../savefile/src

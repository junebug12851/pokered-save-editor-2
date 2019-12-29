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

SOURCES += \
#        src/old/data/gamedata.cpp \
#        src/old/view/mainwindow.cpp \
#        src/old/data/filemanagement.cpp \
  src/data/eventpokemon.cpp \
  src/data/events.cpp \
  src/data/fly.cpp \
  src/data/fonts.cpp \
  src/data/gamedata.cpp \
  src/data/hiddenCoins.cpp \
        src/main.cpp \
#        src/old/data/rawsavedata.cpp

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

HEADERS += \
#    src/old/data/gamedata.h \
    src/common/types.h \
#    src/old/view/mainwindow.h \
#    src/old/data/filemanagement.h \
 \#    src/old/data/rawsavedata.h
  src/data/eventpokemon.h \
  src/data/events.h \
  src/data/fly.h \
  src/data/fonts.h \
  src/data/gamedata.h \
  src/data/hiddenCoins.h \
  src/data/lists/city_list.h \
  src/data/lists/font_list.h \
  src/data/lists/item_list.h \
  src/data/lists/map_list.h \
  src/data/lists/move_list.h \
  src/data/lists/pokemon_listInternal.h \
  src/data/lists/pokemon_listPokedex.h \
  src/data/lists/script_list.h \
  src/data/lists/sprites_list.h \
  src/data/lists/tmHm_list.h \
  src/data/lists/trainers_list.h \
  src/data/lists/types_list.h

# FORMS += \
#    src/old/view/mainwindow.ui

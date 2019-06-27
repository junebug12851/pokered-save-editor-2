QT += quick quickcontrols2 core widgets quickwidgets
CONFIG += c++17

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
        src/model/basemodel.cpp \
        src/model/item.cpp \
        src/model/move.cpp \
        src/model/pokemon.cpp \
        src/model/type.cpp \
        src/store/pokemondatabase.cpp \
        src/view/mainwindow.cpp \
        src/data/filemanagement.cpp \
        src/main.cpp \
        src/data/rawsavedata.cpp

RESOURCES += qml.qrc

RC_ICONS = icons/icon.ico

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    src/model/basemodel.h \
    src/model/item.h \
    src/model/move.h \
    src/model/pokemon.h \
    src/model/type.h \
    src/store/pokemondatabase.h \
    src/includes/types.h \
    src/view/mainwindow.h \
    src/data/filemanagement.h \
    src/data/rawsavedata.h

FORMS += \
    src/view/mainwindow.ui

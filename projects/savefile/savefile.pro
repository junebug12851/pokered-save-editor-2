QT -= gui

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++1z

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/pse-savefile/savefile.cpp

HEADERS += \
    src/pse-savefile/savefile.h

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

#PRE_TARGETDEPS += $$OUT_PWD/../common/libcommon.a

LIBS += \
    -L$$OUT_PWD/../common/ -lcommon \
    -L$$OUT_PWD/../db/ -ldb

INCLUDEPATH += \
    $$PWD/../common/src \
    $$PWD/../db/src

DEPENDPATH += \
    $$PWD/../common/src \
    $$PWD/../db/src

QT = core

TEMPLATE = lib
DEFINES += COMMON_LIBRARY

CONFIG += c++1z

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    src/pse-common/random.cpp \
    src/pse-common/utility.cpp

HEADERS += \
    src/pse-common/common_autoport.h \
    src/pse-common/random.h \
    src/pse-common/types.h \
    src/pse-common/utility.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

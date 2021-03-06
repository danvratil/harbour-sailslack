QT += testlib

include(../defaults.pri)

SRCDIR = $$PWD/../../src

SOURCES += \
    $$PWD/common/modeltest.cpp \
    $$PWD/common/modelwatcher.cpp

HEADERS += \
    $$PWD/common/modeltest.h \
    $$PWD/common/modelwatcher.h

INCLUDEPATH += \
    $$PWD/common

target.path = /opt/tests/harbour-sailslack/
INSTALLS += target

CONFIG = sailfishapp qt testcase
QT += testlib

QMAKE_CXXFLAGS += --std=c++17

INCLUDEPATH += ../../src

HEADERS += \
    ../../src/models/messagemodel.h

SOURCES += \
    ../../src/models/messagemodel.cpp \
    messagemodeltest.cpp

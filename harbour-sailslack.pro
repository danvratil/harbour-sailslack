TEMPLATE = subdirs
SUBDIRS += src tests

CONFIG += sailfishapp_i18n

# The .desktop file
desktop.files = harbour-sailslack.desktop
desktop.path = /usr/share/applications

# Flobal C++ flags
QMAKE_CXXFLAGS += --std=c++17

OTHER_FILES += \
    rpm/*

INSTALLS += desktop harbour-sailslack

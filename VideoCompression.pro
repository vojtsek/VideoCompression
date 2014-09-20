TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    commands.cpp \
    common.cpp \
    sources.cpp \
    networking.cpp

HEADERS += \
    commands.h \
    common.h \
    defines.h \
    networking.h

QMAKE_CXXFLAGS += -std=c++11 \
    -pthread

QMAKE_CFLAGS += -pthread

LIBS += -pthread
unix|win32: LIBS += -lcurses

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    commands.cpp \
    common.cpp \
    networking.cpp \
    sources.cpp \
    handle_IO.cpp \
    network_helper.cpp

HEADERS += \
    commands.h \
    common.h \
    defines.h \
    networking.h \
    handle_IO.h \
    network_helper.h

QMAKE_CXXFLAGS += -std=c++11 \
    -pthread \
-fpermissive

QMAKE_CFLAGS += -pthread

LIBS += -pthread
unix|win32: LIBS += -lcurses

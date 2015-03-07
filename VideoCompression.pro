TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    commands.cpp \
    utilities.cpp \
    sources.cpp \
    handle_IO.cpp \
    network_helper.cpp \
    networkhandler.cpp \
    commands/interactive.cpp \
    commands/maintanance.cpp \
    commands/transfer.cpp

HEADERS += \
    commands.h \
    include_list.h \
    defines.h \
    networkhandler.h \
    handle_IO.h \
    network_helper.h \
    commands/interactive.h \
    include_list.h \
    commands/maintanance.h \
    commands/transfer.h \
    utilities.h

QMAKE_CXXFLAGS += -std=c++11 \
    -pthread \
-fpermissive

QMAKE_CFLAGS += -pthread

LIBS += -pthread
unix|win32: LIBS += -lcurses

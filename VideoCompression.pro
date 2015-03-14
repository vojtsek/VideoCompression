TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    commands.cpp \
    utilities.cpp \
    handle_IO.cpp \
    networkhandler.cpp \
    commands/interactive.cpp \
    commands/maintanance.cpp \
    commands/transfer.cpp \
    structures/TransferInfo.cpp \
    structures/NeighborInfo.cpp \
    structures/WindowPrinter.cpp \
    structures/VideoState.cpp \
    structures/HistoryStorage.cpp \
    structures/MyAddr.cpp \
    helpers/chunk_helper.cpp \
    helpers/network_helper.cpp \
    helpers/filesystem_helper.cpp

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
    utilities.h \
    structures.h \
    structures/VideoState.h \
    structures/WindowPrinter.h \
    enums_types.h \
    templates.h \
    structures/singletons.h \
    structures/HistoryStorage.h \
    structures/structures.h \
    structures/NeighborInfo.h \
    structures/TransferInfo.h \
    helpers/chunk_helper.h \
    helpers/network_helper.h \
    helpers/filesystem_helper.h

QMAKE_CXXFLAGS += -std=c++11 \
    -pthread \
-fpermissive

QMAKE_CFLAGS += -pthread

LIBS += -pthread
unix|win32: LIBS += -lcurses

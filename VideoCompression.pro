TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES +=  \
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
    helpers/OShelper.cpp \
    sources/commands.cpp \
    sources/handle_IO.cpp \
    sources/main.cpp \
    sources/utilities.cpp \
    structures/NeighborStorage.cpp \
    helpers/senders_receivers.cpp \
    structures/NetworkHandler.cpp

HEADERS += \
    commands/interactive.h \
    commands/maintanance.h \
    commands/transfer.h \
    structures/VideoState.h \
    structures/WindowPrinter.h \
    structures/singletons.h \
    structures/HistoryStorage.h \
    structures/structures.h \
    structures/NeighborInfo.h \
    structures/TransferInfo.h \
    helpers/chunk_helper.h \
    helpers/network_helper.h \
    helpers/OShelper.h \
    headers/commands.h \
    headers/defines.h \
    headers/enums_types.h \
    headers/handle_IO.h \
    headers/include_list.h \
    headers/templates.h \
    headers/utilities.h \
    structures/SynchronizedQueue.h \
    structures/NeighborStorage.h \
    helpers/senders_receivers.h \
    structures/NetworkHandler.h

QMAKE_CXXFLAGS += -std=c++11 \
    -pthread \
-fpermissive

QMAKE_CFLAGS += -pthread

LIBS += -pthread
unix|win32: LIBS += -lcurses

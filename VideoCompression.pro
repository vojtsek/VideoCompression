TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    commands.cpp \
    common.cpp

HEADERS += \
    commands.h \
    common.h \
    defines.h

QMAKE_CXXFLAGS += -std=c++11


unix|win32: LIBS += -lcurses

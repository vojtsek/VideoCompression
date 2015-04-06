#############################################################################
# Makefile for building: VideoCompression
# Generated by qmake (3.0) (Qt 5.2.1)
# Project:  ./VideoCompression.pro
# Template: app
# Command: /usr/lib/x86_64-linux-gnu/qt5/bin/qmake -spec linux-g++-64 CONFIG+=debug CONFIG+=declarative_debug CONFIG+=qml_debug -o Makefile ./VideoCompression.pro
#############################################################################

MAKEFILE      = Makefile

####### Compiler, tools and options

CC            = gcc
CXX           = g++
DEFINES       = -DQT_QML_DEBUG -DQT_DECLARATIVE_DEBUG
CFLAGS        = -m64 -pipe -pthread -g -Wall -W -fPIE $(DEFINES)
CXXFLAGS      = -m64 -pipe -std=c++11 -pthread -fpermissive -g -Wall -W -fPIE $(DEFINES)
INCPATH       = -I/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++-64 -I. -I.
LINK          = g++
LFLAGS        = -m64
LIBS          = $(SUBLIBS) -pthread -lcurses 
AR            = ar cqs
RANLIB        = 
QMAKE         = /usr/lib/x86_64-linux-gnu/qt5/bin/qmake
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = cp -f
COPY_DIR      = cp -f -R
STRIP         = strip
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = ./commands/interactive.cpp \
		./commands/maintanance.cpp \
		./commands/transfer.cpp \
		./structures/TransferInfo.cpp \
		./structures/NeighborInfo.cpp \
		./structures/WindowPrinter.cpp \
		./structures/VideoState.cpp \
		./structures/HistoryStorage.cpp \
		./structures/MyAddr.cpp \
		./helpers/chunk_helper.cpp \
		./helpers/network_helper.cpp \
		./helpers/OShelper.cpp \
		./sources/commands.cpp \
		./sources/handle_IO.cpp \
		./sources/main.cpp \
		./sources/utilities.cpp \
		./structures/NeighborStorage.cpp \
		./helpers/senders_receivers.cpp \
		./structures/NetworkHandler.cpp 
OBJECTS       = interactive.o \
		maintanance.o \
		transfer.o \
		TransferInfo.o \
		NeighborInfo.o \
		WindowPrinter.o \
		VideoState.o \
		HistoryStorage.o \
		MyAddr.o \
		chunk_helper.o \
		network_helper.o \
		OShelper.o \
		commands.o \
		handle_IO.o \
		main.o \
		utilities.o \
		NeighborStorage.o \
		senders_receivers.o \
		NetworkHandler.o
DIST          = /usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/spec_pre.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/shell-unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/linux.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/gcc-base.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/gcc-base-unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/g++-base.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/g++-unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/qconfig.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_bootstrap_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_concurrent.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_concurrent_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_core.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_core_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_dbus.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_dbus_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_gui.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_gui_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_network.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_network_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_opengl.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_opengl_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_openglextensions.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_openglextensions_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_platformsupport_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_printsupport.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_printsupport_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_qml.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_qmltest.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_quick.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_sql.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_sql_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_testlib.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_testlib_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_widgets.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_widgets_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_xml.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_xml_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qt_functions.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qt_config.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++-64/qmake.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/spec_post.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/exclusive_builds.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/default_pre.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/resolve_config.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/default_post.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qml_debug.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/declarative_debug.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/warn_on.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/testcase_targets.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/exceptions.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/yacc.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/lex.prf \
		./VideoCompression.pro \
		./VideoCompression.pro
QMAKE_TARGET  = VideoCompression
DESTDIR       = #avoid trailing-slash linebreak
TARGET        = VideoCompression


first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)
	{ test -n "$(DESTDIR)" && DESTDIR="$(DESTDIR)" || DESTDIR=.; } && test $$(gdb --version | sed -e 's,[^0-9][^0-9]*\([0-9]\)\.\([0-9]\).*,\1\2,;q') -gt 72 && gdb --nx --batch --quiet -ex 'set confirm off' -ex "save gdb-index $$DESTDIR" -ex quit '$(TARGET)' && test -f $(TARGET).gdb-index && objcopy --add-section '.gdb_index=$(TARGET).gdb-index' --set-section-flags '.gdb_index=readonly' '$(TARGET)' '$(TARGET)' && rm -f $(TARGET).gdb-index || true

Makefile: ./VideoCompression.pro /usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++-64/qmake.conf /usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/spec_pre.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/shell-unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/linux.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/gcc-base.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/gcc-base-unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/g++-base.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/g++-unix.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/qconfig.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_bootstrap_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_concurrent.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_concurrent_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_core.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_core_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_dbus.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_dbus_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_gui.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_gui_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_network.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_network_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_opengl.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_opengl_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_openglextensions.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_openglextensions_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_platformsupport_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_printsupport.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_printsupport_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_qml.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_qmltest.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_quick.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_sql.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_sql_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_testlib.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_testlib_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_widgets.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_widgets_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_xml.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_xml_private.pri \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qt_functions.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qt_config.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++-64/qmake.conf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/spec_post.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/exclusive_builds.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/default_pre.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/resolve_config.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/default_post.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qml_debug.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/declarative_debug.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/unix/gdb_dwarf_index.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/warn_on.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/testcase_targets.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/exceptions.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/yacc.prf \
		/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/lex.prf \
		./VideoCompression.pro
	$(QMAKE) -spec linux-g++-64 CONFIG+=debug CONFIG+=declarative_debug CONFIG+=qml_debug -o Makefile ./VideoCompression.pro
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/spec_pre.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/shell-unix.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/unix.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/linux.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/gcc-base.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/gcc-base-unix.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/g++-base.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/common/g++-unix.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/qconfig.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_bootstrap_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_concurrent.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_concurrent_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_core.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_core_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_dbus.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_dbus_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_gui.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_gui_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_network.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_network_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_opengl.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_opengl_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_openglextensions.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_openglextensions_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_platformsupport_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_printsupport.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_printsupport_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_qml.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_qmltest.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_quick.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_sql.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_sql_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_testlib.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_testlib_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_widgets.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_widgets_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_xml.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/modules/qt_lib_xml_private.pri:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qt_functions.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qt_config.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/linux-g++-64/qmake.conf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/spec_post.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/exclusive_builds.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/default_pre.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/resolve_config.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/default_post.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/qml_debug.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/declarative_debug.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/unix/gdb_dwarf_index.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/warn_on.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/testcase_targets.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/exceptions.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/yacc.prf:
/usr/lib/x86_64-linux-gnu/qt5/mkspecs/features/lex.prf:
./VideoCompression.pro:
qmake: FORCE
	@$(QMAKE) -spec linux-g++-64 CONFIG+=debug CONFIG+=declarative_debug CONFIG+=qml_debug -o Makefile ./VideoCompression.pro

qmake_all: FORCE

dist: 
	@test -d .tmp/VideoCompression1.0.0 || mkdir -p .tmp/VideoCompression1.0.0
	$(COPY_FILE) --parents $(SOURCES) $(DIST) .tmp/VideoCompression1.0.0/ && (cd `dirname .tmp/VideoCompression1.0.0` && $(TAR) VideoCompression1.0.0.tar VideoCompression1.0.0 && $(COMPRESS) VideoCompression1.0.0.tar) && $(MOVE) `dirname .tmp/VideoCompression1.0.0`/VideoCompression1.0.0.tar.gz . && $(DEL_FILE) -r .tmp/VideoCompression1.0.0


clean:compiler_clean 
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~ core *.core


####### Sub-libraries

distclean: clean
	-$(DEL_FILE) $(TARGET) 
	-$(DEL_FILE) Makefile


check: first

compiler_yacc_decl_make_all:
compiler_yacc_decl_clean:
compiler_yacc_impl_make_all:
compiler_yacc_impl_clean:
compiler_lex_make_all:
compiler_lex_clean:
compiler_clean: 

####### Compile

interactive.o: ./commands/interactive.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h \
		./rapidjson/document.h \
		./rapidjson/reader.h \
		./rapidjson/rapidjson.h \
		./rapidjson/msinttypes/stdint.h \
		./rapidjson/msinttypes/inttypes.h \
		./rapidjson/allocators.h \
		./rapidjson/encodings.h \
		./rapidjson/internal/meta.h \
		./rapidjson/internal/pow10.h \
		./rapidjson/internal/stack.h \
		./rapidjson/error/error.h \
		./rapidjson/internal/strfunc.h \
		./rapidjson/prettywriter.h \
		./rapidjson/writer.h \
		./rapidjson/internal/dtoa.h \
		./rapidjson/internal/itoa.h \
		./rapidjson/stringbuffer.h \
		./rapidjson/filestream.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o interactive.o ./commands/interactive.cpp

maintanance.o: ./commands/maintanance.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o maintanance.o ./commands/maintanance.cpp

transfer.o: ./commands/transfer.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o transfer.o ./commands/transfer.cpp

TransferInfo.o: ./structures/TransferInfo.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o TransferInfo.o ./structures/TransferInfo.cpp

NeighborInfo.o: ./structures/NeighborInfo.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o NeighborInfo.o ./structures/NeighborInfo.cpp

WindowPrinter.o: ./structures/WindowPrinter.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o WindowPrinter.o ./structures/WindowPrinter.cpp

VideoState.o: ./structures/VideoState.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o VideoState.o ./structures/VideoState.cpp

HistoryStorage.o: ./structures/HistoryStorage.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o HistoryStorage.o ./structures/HistoryStorage.cpp

MyAddr.o: ./structures/MyAddr.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o MyAddr.o ./structures/MyAddr.cpp

chunk_helper.o: ./helpers/chunk_helper.cpp ./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o chunk_helper.o ./helpers/chunk_helper.cpp

network_helper.o: ./helpers/network_helper.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o network_helper.o ./helpers/network_helper.cpp

OShelper.o: ./helpers/OShelper.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o OShelper.o ./helpers/OShelper.cpp

commands.o: ./sources/commands.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o commands.o ./sources/commands.cpp

handle_IO.o: ./sources/handle_IO.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o handle_IO.o ./sources/handle_IO.cpp

main.o: ./sources/main.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o main.o ./sources/main.cpp

utilities.o: ./sources/utilities.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o utilities.o ./sources/utilities.cpp

NeighborStorage.o: ./structures/NeighborStorage.cpp ./structures/NeighborStorage.h \
		./headers/enums_types.h \
		./structures/TransferInfo.h \
		./structures/structures.h \
		./headers/utilities.h \
		./headers/commands.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborInfo.h \
		./helpers/network_helper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o NeighborStorage.o ./structures/NeighborStorage.cpp

senders_receivers.o: ./helpers/senders_receivers.cpp ./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o senders_receivers.o ./helpers/senders_receivers.cpp

NetworkHandler.o: ./structures/NetworkHandler.cpp ./structures/NetworkHandler.h \
		./headers/defines.h \
		./headers/enums_types.h \
		./structures/WindowPrinter.h \
		./structures/singletons.h \
		./structures/SynchronizedQueue.h \
		./headers/handle_IO.h \
		./structures/NeighborStorage.h \
		./structures/structures.h \
		./structures/NeighborInfo.h \
		./headers/include_list.h \
		./commands/interactive.h \
		./headers/commands.h \
		./commands/maintanance.h \
		./commands/transfer.h \
		./headers/utilities.h \
		./headers/templates.h \
		./structures/HistoryStorage.h \
		./structures/VideoState.h \
		./structures/TransferInfo.h \
		./helpers/chunk_helper.h \
		./helpers/network_helper.h \
		./helpers/senders_receivers.h \
		./helpers/OShelper.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o NetworkHandler.o ./structures/NetworkHandler.cpp

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:


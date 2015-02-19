#include "networking.h"

#include <string>
#include <iostream>
#include <sstream>

#ifndef COMMANDS_H
#define COMMANDS_H

#ifndef DEFINES_H
#include "defines.h"
#endif

using namespace std;

class Command {
protected:
    VideoState *state;
public:
    Command(VideoState *st) {
        state = st;
    }
    virtual void execute() = 0;
    virtual ~Command() {}
};

class CmdDef: public Command {
public:
    CmdDef(): Command(nullptr) {}
    virtual void execute();
};

class NetworkCommand: public Command {
protected:
    NetworkHandle *handler;
public:
    NeighborInfo *peer;
    int fd;
    int connectPeer(struct sockaddr_storage *addr);
    NetworkCommand(VideoState *state, int fildes, NetworkHandle *nhandler):
        Command(state), handler(nhandler), fd(fildes) {}
    virtual void execute();
};

class CmdHelp: public Command {
public:
    CmdHelp(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdShow: public NetworkCommand {
public:
    CmdShow(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdStart: public Command {
public:
    CmdStart(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdSet: public Command {
public:
    CmdSet(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdLoad: public Command {
public:
    CmdLoad(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdSetCodec: public Command {
public:
    CmdSetCodec(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdSetChSize: public Command {
public:
    CmdSetChSize(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdAsk: public NetworkCommand {
public:
    CmdAsk(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdRespond: public NetworkCommand {
public:
    CmdRespond(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdReact: public NetworkCommand {
public:
    CmdReact(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdConfirmPeer: public NetworkCommand {
public:
    CmdConfirmPeer(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdConfirmHost: public NetworkCommand {
public:
    CmdConfirmHost(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdAskHost: public NetworkCommand {
public:
    CmdAskHost(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

class CmdAskPeer: public NetworkCommand {
public:
    CmdAskPeer(VideoState *st, int fd, NetworkHandle *hndl): NetworkCommand(st, fd, hndl) {}
    virtual void execute();
};

#endif

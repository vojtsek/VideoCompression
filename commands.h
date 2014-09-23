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
    virtual void execute();
    virtual ~Command() {}
};

class CmdHelp: public Command {
public:
    CmdHelp(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdShow: public Command {
public:
    CmdShow(VideoState *st): Command(st) {}
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

class NetworkCommand: public Command {
protected:
    NetworkHandle *handler;
public:
    int connectPeer(struct sockaddr_storage *addr);
    int fd;
    NetworkCommand(VideoState *state, int fildes, NetworkHandle *nhandler):
        Command(state), handler(nhandler), fd(fildes) {}
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

#endif

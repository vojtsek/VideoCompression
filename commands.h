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
    int connectPeer(struct sockaddr_storage *addr);
    NetworkCommand(VideoState *state, NetworkHandle *nhandler):
        Command(state), handler(nhandler) {}
    virtual void execute() {}
    virtual bool execute(int fd, struct sockaddr_storage &addr, void *data) = 0;
};

class CmdHelp: public Command {
public:
    CmdHelp(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdShow: public NetworkCommand {
public:
    CmdShow(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data) {}
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
    CmdAsk(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdRespond: public NetworkCommand {
public:
    CmdRespond(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdReact: public NetworkCommand {
public:
    CmdReact(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdConfirmPeer: public NetworkCommand {
public:
    CmdConfirmPeer(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdConfirmHost: public NetworkCommand {
public:
    CmdConfirmHost(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdAskHost: public NetworkCommand {
public:
    CmdAskHost(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdAskPeer: public NetworkCommand {
public:
    CmdAskPeer(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdPingPeer: public NetworkCommand {
public:
    CmdPingPeer(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdPingHost: public NetworkCommand {
public:
    CmdPingHost(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdTransferPeer: public NetworkCommand {
public:
    CmdTransferPeer(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdTransferHost: public NetworkCommand {
public:
    CmdTransferHost(VideoState *st, NetworkHandle *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};


#endif

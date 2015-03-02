#include "commands.h"
#ifndef MAINTANANCE_H
#define MAINTANANCE_H

class CmdConfirmPeer: public NetworkCommand {
public:
    CmdConfirmPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdConfirmHost: public NetworkCommand {
public:
    CmdConfirmHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdAskHost: public NetworkCommand {
public:
    CmdAskHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdAskPeer: public NetworkCommand {
public:
    CmdAskPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdPingPeer: public NetworkCommand {
public:
    CmdPingPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdPingHost: public NetworkCommand {
public:
    CmdPingHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

#endif // MAINTANANCE_H

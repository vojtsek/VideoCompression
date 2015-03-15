#include "headers/commands.h"
#ifndef MAINTANANCE_H
#define MAINTANANCE_H

class CmdConfirmPeer: public NetworkCommand {
public:
    CmdConfirmPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string printName() {
        return("Confirm Peer");
    }
};

class CmdConfirmHost: public NetworkCommand {
public:
    CmdConfirmHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string printName() {
        return("Confirm Host");
    }
};

class CmdAskHost: public NetworkCommand {
public:
    CmdAskHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string printName() {
        return("Ask Host");
    }
};

class CmdAskPeer: public NetworkCommand {
public:
    CmdAskPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string printName() {
        return("Ask Peer");
    }
};

class CmdPingPeer: public NetworkCommand {
public:
    CmdPingPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string printName() {
        return("Ping Peer");
    }
};

class CmdPingHost: public NetworkCommand {
public:
    CmdPingHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string printName() {
        return("Ping Host");
    }
};

#endif // MAINTANANCE_H

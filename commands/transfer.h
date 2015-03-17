#include "headers/commands.h"
#ifndef TRANSFER_CMD_H
#define TRANSFER_CMD_H

class CmdDistributePeer: public NetworkCommand {
public:
    CmdDistributePeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Distr Peer");
    }
};

class CmdDistributeHost: public NetworkCommand {
public:
    CmdDistributeHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Distr Host");
    }
};

class CmdReturnPeer: public NetworkCommand {
public:
    CmdReturnPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Return Peer");
    }
};

class CmdReturnHost: public NetworkCommand {
public:
    CmdReturnHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Return Host");
    }
};

#endif

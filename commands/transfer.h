#include "commands.h"
#ifndef TRANSFER_CMD_H
#define TRANSFER_CMD_H

class CmdTransferPeer: public NetworkCommand {
public:
    CmdTransferPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};

class CmdTransferHost: public NetworkCommand {
public:
    CmdTransferHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
};


#endif

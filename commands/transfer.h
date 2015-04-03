#include "headers/commands.h"
#ifndef TRANSFER_CMD_H
#define TRANSFER_CMD_H

/*!
 * \brief The CmdDistributePeer class
 * responds, depending whether it is ready or not; if yes,
 * accepts information about chunk and begins the file transfer
 * then pushes the chunk to processing queue and updates status
 */
class CmdDistributePeer: public NetworkCommand {
public:
    CmdDistributePeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Distr Peer");
    }
};

/*!
 * \brief The CmdDistributeHost class
 * contacts given peer, determines its readiness
 * if peer is busy, updates its status
 * otherwise it sends the chunk to be encoded
 */
class CmdDistributeHost: public NetworkCommand {
public:
    CmdDistributeHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Distr Host");
    }
};

/*!
 * \brief The CmdReturnPeer class
 * receives the computed chunk,
 * calls utilities::chunkhelper::processReturnedChunk
 */
class CmdReturnPeer: public NetworkCommand {
public:
    CmdReturnPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Return peer");
    }
};

/*!
 * \brief The CmdReturnHost class
 * sends the encoded chunk back to the peer
 */
class CmdReturnHost: public NetworkCommand {
public:
    CmdReturnHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Return host");
    }
};

/*!
 * \brief The CmdGatherNeighborsPeer class
 * receives address of the asking node and pings it
 * decreases the TTL value and if not null,
 * spreads the adress to its neighbors, uses NetworkHandler::gatherNeighbors()
 */
class CmdGatherNeighborsPeer: public NetworkCommand {
public:
    CmdGatherNeighborsPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Gather neighbors peer");
    }
};

/*!
 * \brief The CmdGatherNeighborsHost class
 * called by NetworkHandler::gatherNeighbors()
 * sends the address thats asking for help
 */
class CmdGatherNeighborsHost: public NetworkCommand {
public:
    CmdGatherNeighborsHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return ("Gather neighbors host");
    }
};

#endif

#include "headers/commands.h"
#ifndef MAINTANANCE_H
#define MAINTANANCE_H

/*!
 * \brief The CmdConfirmPeer class
 * command invoked on the peer side of the connection, being confirmed
 * spawns connection to the host, determines used adress
 * and sends it, i.e. its confirmed
 */
class CmdConfirmPeer: public NetworkCommand {
public:
    CmdConfirmPeer(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Confirm Peer");
    }
};

/*!
 * \brief The CmdConfirmHost class
 * the host wants to confirm its neighbor
 * it contacts it, sends its port and awaits the adress as the confirmation
 * then it adds the peer as the neighbor
 */
class CmdConfirmHost: public NetworkCommand {
public:
    CmdConfirmHost(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Confirm Host");
    }
};

/*!
 * \brief The CmdAskHost class
 * host asks, receives count of addresses that will be send
 * followed by series of address strcutures
 */
class CmdAskHost: public NetworkCommand {
public:
    CmdAskHost(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ask Host");
    }
};

/*!
 * \brief The CmdAskPeer class
 * peer is asked for addresses of some neighbors
 * it picks some of them and sends the count followeb by
 * series of address structures
 */
class CmdAskPeer: public NetworkCommand {
public:
    CmdAskPeer(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ask Peer");
    }
};

/*!
 * \brief The CmdPingPeer class
 * maintaining command
 * peer receives host's state and port, sends its state
 * then updates its neighbor database
 */
class CmdPingPeer: public NetworkCommand {
public:
    CmdPingPeer(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ping Peer");
    }
};

/*!
 * \brief The CmdPingHost class
 * host pings peer, so it can update its neighbor database
 * host also gathers information about peer state
 */
class CmdPingHost: public NetworkCommand {
public:
    CmdPingHost(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ping Host");
    }
};

/*!
 * \brief The CmdCancelPeer class
 * the host informs the peer, that it no longer need to process particular chunk
 */
class CmdCancelPeer: public NetworkCommand {
public:
    CmdCancelPeer(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Cancel Peer");
    }
};

/*!
 * \brief The CmdCancelHost class
 * the host informs the peer, that it no longer need to process particular chunk
 */
class CmdCancelHost: public NetworkCommand {
public:
    CmdCancelHost(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Cancel Host");
    }
};


/*!
 * \brief The CmdGoodbyePeer class
 * peer reads the host's port, removes the host from its database
 * and sends confirmative response
 */
class CmdGoodbyePeer: public NetworkCommand {
public:
    CmdGoodbyePeer(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Goodbye Peer");
    }
};


/*!
 * \brief The CmdGoodbyeHost class
 * contacts the peer, sends its ports and waits for confirmation
 */
class CmdGoodbyeHost: public NetworkCommand {
public:
    CmdGoodbyeHost(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Goodbye Host");
    }
};

/*!
 * \brief The CmdSayGoodbye class
 * traverse neighbors and says goodbye to everyone
 */
class CmdSayGoodbye: public NetworkCommand {
public:
    CmdSayGoodbye(TaskHandler *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t , struct sockaddr_storage &, void *) { return true; }
    virtual void execute();

    virtual std::string getName() {
        return("Goodbye...");
    }
};

#endif // MAINTANANCE_H

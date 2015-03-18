#include "headers/commands.h"
#ifndef MAINTANANCE_H
#define MAINTANANCE_H

class CmdConfirmPeer: public NetworkCommand {
public:
    CmdConfirmPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Confirm Peer");
    }
};

class CmdConfirmHost: public NetworkCommand {
public:
    CmdConfirmHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Confirm Host");
    }
};

class CmdAskHost: public NetworkCommand {
public:
    CmdAskHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ask Host");
    }
};

class CmdAskPeer: public NetworkCommand {
public:
    CmdAskPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ask Peer");
    }
};

class CmdPingPeer: public NetworkCommand {
public:
    CmdPingPeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ping Peer");
    }
};

class CmdPingHost: public NetworkCommand {
public:
    CmdPingHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Ping Host");
    }
};

class CmdGoodbyePeer: public NetworkCommand {
public:
    CmdGoodbyePeer(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Goodbye Peer");
    }
};

class CmdGoodbyeHost: public NetworkCommand {
public:
    CmdGoodbyeHost(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int fd, struct sockaddr_storage &address, void *data);
    virtual std::string getName() {
        return("Goodbye Host");
    }
};

class CmdSayGoodbye: public NetworkCommand {
public:
    CmdSayGoodbye(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int , struct sockaddr_storage &, void *) { return true; }
    virtual void execute();

    virtual std::string getName() {
        return("Goodbye...");
    }
};

#endif // MAINTANANCE_H

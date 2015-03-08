#include "networkhandler.h"

#include <string>
#include <iostream>
#include <sstream>

#ifndef COMMANDS_H
#define COMMANDS_H

#ifndef DEFINES_H
#include "defines.h"
#endif

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

class NetworkCommand: public Command {
protected:
    NetworkHandler *handler;
public:
    int connectPeer(struct sockaddr_storage *addr);
    NetworkCommand(VideoState *state, NetworkHandler *nhandler):
        Command(state), handler(nhandler) {}
    virtual void execute() {}
    virtual bool execute(int fd, struct sockaddr_storage &addr, void *data) = 0;
};

#endif

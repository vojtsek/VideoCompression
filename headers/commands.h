#include "headers/networkhandler.h"

#include <string>
#include <iostream>
#include <sstream>

#ifndef COMMANDS_H
#define COMMANDS_H


class VideoState;
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
    int32_t connectPeer(struct sockaddr_storage *addr);
    NetworkCommand(VideoState *state, NetworkHandler *nhandler):
        Command(state), handler(nhandler) {}
    virtual std::string getName() {
        return(typeid(this).name());
    }
    virtual void execute() {}
    virtual bool execute(int32_t fd, struct sockaddr_storage &addr, void *data) = 0;
};

#endif

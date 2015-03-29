#include "headers/commands.h"
#ifndef INTER_CMD_H
#define INTER_CMD_H

class CmdDef: public Command {
public:
    CmdDef(): Command(nullptr) {}
    virtual void execute();
};

class CmdHelp: public Command {
public:
    CmdHelp(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdScrollUp: public Command {
public:
    CmdScrollUp(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdScrollDown: public Command {
public:
    CmdScrollDown(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdShow: public NetworkCommand {
public:
    CmdShow(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int, struct sockaddr_storage &, void *) { return true;}
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

class CmdSetFormat: public Command {
public:
    CmdSetFormat(VideoState *st): Command(st) {}
    virtual void execute();
};

class CmdSetChSize: public Command {
public:
    CmdSetChSize(VideoState *st): Command(st) {}
    virtual void execute();
};


#endif

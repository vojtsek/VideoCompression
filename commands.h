#include <string>
#include <iostream>
#include <sstream>

#ifndef COMMANDS_H
#define COMMANDS_H

#ifndef DEFINES_H
#include "defines.h"
#endif

using namespace std;

class Command {
public:
    virtual void execute(stringstream &ss, State &state);
    virtual ~Command() {}
};

class CmdHelp: public Command {
public:
    virtual void execute(stringstream &ss, State &state);
};

class CmdShow: public Command {
public:
    virtual void execute(stringstream &ss, State &state);
};

class CmdStart: public Command {
public:
    virtual void execute(stringstream &ss, State &state);
};

class CmdSet: public Command {
public:
    virtual void execute(stringstream &ss, State &state);
};

class CmdLoad: public Command {
public:
    virtual void execute(stringstream &ss, State &state);
};

#endif

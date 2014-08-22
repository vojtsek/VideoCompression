#include <string>
#include <iostream>
#include <sstream>

#include "defines.h"

#ifndef COMMANDS_H
#define COMMANDS_H

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

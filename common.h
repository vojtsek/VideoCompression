#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "defines.h"
#include "commands.h"

#ifndef COMMON_H
#define COMMON_H
#define CSI "\033["
class Command;
namespace common
{
    int readCmd(std::istream &ins, std::map<std::string, Command *> &cmds, State &st);
    void listCmds();
    int checkFile(std::string &path);
    std::vector<string> extract(std::string &text, std::string &from, int count);
    std::ostream &red(std::ostream &out);
    std::ostream &green(std::ostream &out);
    std::ostream &black(std::ostream &out);
    std::ostream &white(std::ostream &out);
    std::ostream &defaultFg(std::ostream &out);
}

#endif

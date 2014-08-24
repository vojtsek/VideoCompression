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
    int prepareDir(std::string &loc);
    int rmrDir(const char *loc, bool rec);
    std::vector<std::string> extract(const std::string text, const std::string from, int count);
    std::string getTimestamp();
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    int runExternal(std::string &o, std::string &e, const std::string &cmd, int numargs, ...);
    void reportError(const std::string &err);
    void printInfo(const std::string &msg);
    std::ostream &red(std::ostream &out);
    std::ostream &green(std::ostream &out);
    std::ostream &black(std::ostream &out);
    std::ostream &white(std::ostream &out);
    std::ostream &gray(std::ostream &out);
    std::ostream &grayBg(std::ostream &out);
    std::ostream &defaultFg(std::ostream &out);
    std::ostream &defaultBg(std::ostream &out);
}

#endif

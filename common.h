#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <string>

#include "commands.h"
#include "defines.h"

#ifndef COMMON_H
#define COMMON_H
#define CSI "\033["
class Command;
namespace common
{
    int readCmd(std::stringstream &ins, cmd_storage_t &cmds, State &st);
    void listCmds();
    void initCurses();
    void cursToInfo();
    void cursToPerc();
    void cursToCmd();
    void cursToStatus();
    int checkFile(std::string &path);
    int prepareDir(std::string &loc);
    int rmrDir(const char *loc, bool rec);
    int getLine(char *line, int len, HistoryStorage &hist);
    std::vector<std::string> extract(const std::string text, const std::string from, int count);
    std::vector<std::string> split(const std::string &content, char sep);
    std::string getTimestamp();
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    int runExternal(bool measure, std::string &o, std::string &e, char *cmd, int numargs, ...);
    void cursorToX(int nx);
    void reportError(const std::string &err);
    void reportSuccess(const std::string &msg);
    void reportStatus(const std::string &msg);
    void printProgress(double percent);
    void reportTime(const std::string &file, double time);
    bool knownCodec(const std::string &cod);
}

#endif

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
    vector<string> getKnownCodecs();
    int readCmd(std::stringstream &ins, cmd_storage_t &cmds, State &st);
    int acceptCmd(cmd_storage_t &cmds, State &st);
    int checkFile(std::string &path);
    int prepareDir(std::string &loc);
    int rmrDir(const char *loc, bool rec);
    int getLine(char *line, int len, const std::string &histf, bool save);
    int runExternal(std::string &o, std::string &e, char *cmd, int numargs, ...);
    long getFileSize(const std::string &file);
    std::vector<std::string> extract(const std::string text, const std::string from, int count);
    std::vector<std::string> split(const std::string &content, char sep);
    std::string getTimestamp();
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    std::string loadInput(const std::string &histf, const std::string &msg, bool save);
    void listCmds();
    void initCurses();
    void cursToInfo();
    void cursToPerc();
    void cursToCmd();
    void cursToStatus();
    void cursToQuestion();
    void clearNlines(int n);
    void reportFileProgress(const std::string &file, long desired);
    void cursorToX(int nx);
    void writeSuccess(const std::string &msg);
    void writeError(const std::string &err);
    void reportError(const std::string &err);
    void reportSuccess(const std::string &msg);
    void reportStatus(const std::string &msg);
    void printProgress(double percent);
    void reportTime(const std::string &file, double time);
    bool knownCodec(const std::string &cod);
    bool isAcceptable(char c);
}

#endif

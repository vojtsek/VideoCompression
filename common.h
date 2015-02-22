#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <string>

#include <unistd.h>

#include "commands.h"
#include "networking.h"
#include "handle_IO.h"
#include "defines.h"

#ifndef COMMON_H
#define COMMON_H
#define CSI "\033["
class Command;
namespace common
{
    vector<string> getKnownCodecs();
    int readCmd(std::stringstream &ins, cmd_storage_t &cmds, VideoState &st);
    int acceptCmd(cmd_storage_t &cmds);
    int checkFile(std::string &path);
    int prepareDir(std::string &loc);
    int rmrDir(const char *loc, bool rec);
    int runExternal(std::string &o, std::string &e, char *cmd, int numargs, ...);
    int encodeChunk(std::string &path, std::string &codec, std::string &extension);
    long getFileSize(const std::string &file);
    std::vector<std::string> extract(const std::string text, const std::string from, int count);
    std::vector<std::string> split(const std::string &content, char sep);
    std::string getTimestamp();
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    void listCmds();
    bool knownCodec(const std::string &cod);
    bool isAcceptable(char c);
    char *m_itoa(int n);
}
#endif

#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

#include "commands.h"
#include "networkhandler.h"
#include "handle_IO.h"
#include "defines.h"

#ifndef UTILS_H
#define UTILS_H
class Command;
namespace utilities
{
    int readCmd(std::stringstream &ins, cmd_storage_t &cmds, VideoState &st);
    int acceptCmd(cmd_storage_t &cmds);
    int checkFile(std::string &path);
    int prepareDir(std::string &loc);
    int rmrDir(const char *loc, bool rec);
    int runExternal(std::string &o, std::string &e, char *cmd, int numargs, ...);
    int encodeChunk(TransferInfo *ti);
    long getFileSize(const std::string &file);
    std::vector<std::string> extract(const std::string text, const std::string from, int count);
    std::vector<std::string> split(const std::string &content, char sep);
    std::vector<std::string> getKnownCodecs();
    std::string getTimestamp();
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    std::string m_itoa(int n);
    std::string getHash(NeighborInfo &n);
    void listCmds();
    bool knownCodec(const std::string &cod);
    bool isAcceptable(char c);

}
#endif

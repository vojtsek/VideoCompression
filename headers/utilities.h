#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

#include "headers/commands.h"
#include "headers/networkhandler.h"
#include "headers/handle_IO.h"
#include "headers/defines.h"

#ifndef UTILS_H
#define UTILS_H
class Command;
class TransferInfo;
namespace utilities
{
    int32_t readCmd(std::stringstream &ins, cmd_storage_t &cmds, VideoState &st);
    int32_t acceptCmd(cmd_storage_t &cmds);
    int32_t computeDuration(std::string t1, std::string t2);
    std::vector<std::string> extract(const std::string text, const std::string from, int32_t count);
    std::vector<std::string> split(const std::string &content, char sep);
    std::vector<std::string> getKnownCodecs();
    std::string getTimestamp();
    std::string m_itoa(int32_t n);
    std::string toString(NeighborInfo &n);
    std::string formatString(std::string str1, std::string str2);
    void listCmds();
    void printOverallState(VideoState *state);
    bool knownCodec(const std::string &cod);
    bool knownFormat(const std::string &format);
    bool isAcceptable(char c);
}
#endif

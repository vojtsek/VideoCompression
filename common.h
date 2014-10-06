#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <string>

#include <unistd.h>

#include "commands.h"
#include "networking.h"
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
    int getLine(char *line, int len, const std::string &histf, bool save);
    int runExternal(std::string &o, std::string &e, char *cmd, int numargs, ...);
    int encodeChunk(std::string &path, std::string &codec, std::string &extension);
    long getFileSize(const std::string &file);
    std::vector<std::string> extract(const std::string text, const std::string from, int count);
    std::vector<std::string> split(const std::string &content, char sep);
    std::string getTimestamp();
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    std::string loadInput(const std::string &histf, const std::string &msg, bool save);
    struct sockaddr_storage addr2storage(const char* addr, int port, int family);
    struct sockaddr_storage getHostAddr(int fd);
    struct sockaddr_storage getPeerAddr(int fd);
    void listCmds();
    void initCurses();
    void cursToInfo();
    void cursToPerc();
    void cursToCmd();
    void cursToStatus();
    void cursToQuestion();
    void clearNlines(int n);
    void clearProgress();
    void reportFileProgress(const std::string &file, long desired);
    void cursorToX(int nx);
    void writeSuccess(const std::string &msg);
    void writeError(const std::string &err);
    void reportError(const std::string &err);
    void reportSuccess(const std::string &msg);
    void reportStatus(const std::string &msg);
    void reportDebug(const std::string &msg, int lvl);
    void printProgress(double percent);
    void reportTime(const std::string &file, double time);
    bool knownCodec(const std::string &cod);
    bool isAcceptable(char c);
    bool addrIn(struct sockaddr_storage &st, std::vector<NeighborInfo> list);
    bool cmpStorages(struct sockaddr_storage &s1, struct sockaddr_storage &s2);
    template<typename T>
    int sendSth(T what, int fd) {
        int w;
        if ((w = write(fd, &what, sizeof (T))) < 1) {
            common::reportError("Problem occured while sending the data.");
            return (-1);
        }
        return (w);
    }

    template<typename T>
    int sendSth(T *what, int fd, int len) {
        int w;
        if ((w = write(fd, what, len * sizeof(T))) < 1) {
            reportError("Problem occured while sending the data.");
            return (-1);
        }
        return (w);
    }
    template<typename T>
    int recvSth(T &where, int fd) {
        int r;
         if ((r = read(fd, &where, sizeof (T))) < 1) {
            reportError("Problem occured while accepting the data.");
            return (-1);
        }
        return (r);
    }
    template<typename T>
    int recvSth(T *where, int fd, int len) {
        int r;
        if ((r = read(fd, where, len * sizeof (T))) < 1) {
            reportError("Problem occured while accepting the data.");
            return (-1);
        }
        return (r);
    }
}

#endif

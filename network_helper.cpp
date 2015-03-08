#define _FILE_OFFSET_BITS 64
#include "include_list.h"
#include "handle_IO.h"
#include "commands.h"
#include "defines.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <curses.h>
#include <deque>
#include <thread>
#include <string.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include "network_helper.h"


bool cmpStorages(struct sockaddr_storage &s1, struct sockaddr_storage &s2) {
    if (((struct sockaddr *) &s1)->sa_family != ((struct sockaddr *) &s2)->sa_family)
        return false;
    if (((struct sockaddr *) &s1)->sa_family == AF_INET) {
        if ((((struct sockaddr_in *) &s1)->sin_port == ((struct sockaddr_in *) &s2)->sin_port))
                //(((struct sockaddr_in *) &s1)->sin_addr == ((struct sockaddr_in *) &s2)->sin_addr))
            return true;
        else
            return false;
    }
    //TODO: maybe bad
    if (((struct sockaddr *) &s1)->sa_family == AF_INET6) {
        if ((((struct sockaddr_in6 *) &s1)->sin6_port == ((struct sockaddr_in6 *) &s2)->sin6_port))
               // (((struct sockaddr_in6 *) &s1)->sin6_addr == ((struct sockaddr_in6 *) &s2)->sin6_addr))
            return true;
        else
            return false;
    }
    return false;
}

bool addrIn(struct sockaddr_storage &addr, std::vector<NeighborInfo *> &list) {

    for (auto n : list) {
        if (cmpStorages(n->address, addr))
            return true;
    }
    return false;
}

//TODO getsockname doesnt fail if short structure was supplied!
void getHostAddr(struct sockaddr_storage &addr, int fd) {
    struct sockaddr_in *in4p = (struct sockaddr_in *) &addr;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &addr;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
    if (DATA->config.IPv4_ONLY) {
        if (getsockname(fd, (struct sockaddr *) in4p, &size4) == -1) {
            reportDebug("Not an IPv4 addr.", 4);
        } else {
            in4p->sin_family = AF_INET;
            return;
        }
    } else {
        in6p->sin6_family = AF_INET6;
        if (getsockname(fd, (struct sockaddr *) in6p, &size6) == -1) {
            reportDebug("Not an IPv6 addr.", 4);
        } else
            return;
    }
    reportDebug("Failed to get host's address.", 2);
    throw new std::exception();
    return;
}

void getPeerAddr(struct sockaddr_storage &addr, int fd) {
    struct sockaddr_in *in4p = (struct sockaddr_in *) &addr;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &addr;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
    if (getpeername(fd, (struct sockaddr *) in4p, &size4) == -1) {
        reportDebug("Not an IPv4 addr.", 4);
    } else {
        addr.ss_family = AF_INET;
        return;
    }
    if (getpeername(fd, (struct sockaddr *) in6p, &size6) == -1) {
        reportDebug("Not an IPv6 addr.", 4);
    } else {
        addr.ss_family = AF_INET6;
        return;
    }
    reportDebug("Failed to get host's address.", 2);
    throw new std::exception();
}

struct sockaddr_storage addr2storage(const char *addrstr, int port, int family) {
    struct sockaddr_storage addr;
    if (family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) &addr;
        addr4->sin_family = family;
        addr4->sin_port = htons(port);
        inet_pton(family, addrstr, &(addr4->sin_addr));
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &addr;
        addr6->sin6_family = family;
        addr6->sin6_port = htons(port);
        inet_pton(family, addrstr, &(addr6->sin6_addr));
    }
    return addr;
}

std::string storage2addr(sockaddr_storage &addr) {
    if (addr.ss_family == AF_INET) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
        return std::string(buf);
    } else {
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN);
        return std::string(buf);
    }
}

int sendString(int fd, std::string str) {
    if (sendSth(str.length(), fd) == -1)
        return (-1);
    for (char c : str) {
        if (sendSth(c, fd) == -1)
            return (-1);
    }
    return (0);
}

int receiveFile(int fd, std::string fn) {
    off_t fsize, received = 0, r, w;
    int o_file;
    char buf[DATA->config.getValue("TRANSFER_BUF_LENGTH")];
    try {
        if ((o_file = open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
            reportDebug(fn + ": Failed to open the output file. ", 2);
            throw 1;
        }

        if (recvSth(fsize, fd) == -1) {
            reportDebug(fn + ": Failed to get file size. ", 2);
            throw 1;
        }
        //check the size
        while ((r = read(fd, buf, DATA->config.getValue("TRANSFER_BUF_LENGTH"))) > 0) {
            //reportError(utilities::m1_itoa(r));
            received += r;
            if ((w = write(o_file, buf, r)) != -1) {
                //reportSuccess(fn + ": " + utilities::m_itoa(r) + " bytes received.");
            } else {
                reportDebug("Error; received " + utilities::m_itoa(received), 2);
                throw 1;
            }
        }
        if (received != fsize) {
            reportDebug("Received bytes and expected fsize does not equal. ", 2);
            reportDebug("EXP: " + utilities::m_itoa(fsize), 1);
            reportDebug("REC: " + utilities::m_itoa(received), 1);
            throw 1;
        }
    } catch (int) {
        reportError("Bad transfer");
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        close(o_file);
        return -1;
    }
    reportDebug("received " + utilities::m_itoa(received), 2);
    close(o_file);
    return 0;
}

int sendFile(int fd, std::string fn) {
    int file;
    off_t fsize, sent = 0, r, w;
    char buf[DATA->config.getValue("TRANSFER_BUF_LENGTH")];
    try {
        if ((file = open(fn.c_str(), O_RDONLY)) == -1) {
            reportDebug(fn + ": Failed to open.", 2);
            throw 1;
        }
        if ((fsize = utilities::getFileSize(fn)) == -1) {
            reportDebug(fn + ": Failed to get file size", 2);
            throw 1;
        }

        if (sendSth(fsize, fd) == -1) {
            reportDebug(fn + ": Failed to send file size", 2);
            throw 1;
        }

        sent = fsize;
        while ((r = read(file, buf, DATA->config.getValue("TRANSFER_BUF_LENGTH"))) > 0) {
            if ((w = write(fd, buf, r)) != -1) {
                sent -= w;
            } else {
                reportDebug("Error; sent " + utilities::m_itoa(w), 2);
                throw 1;
            }
        }

        if (sent) {
            reportDebug("Sent bytes and filesize does not equal", 2);
            throw 1;
        }
    } catch (int) {
        close(file);
        return -1;
    }
    close(file);
    return 0;
}

int sendCmd(int fd, CMDS cmd) {
    bool response;
    if (sendSth(cmd, fd) == -1) {
        reportError("Error send CMD");
        return (-1);
    }
    if (recvSth(response, fd) == -1) {
        reportError("ERROR conf CMD");
        return (-1);
    }
    if (!response) {
        reportDebug("The command was not accepted.", 1);
        return (-1);
    }
    return (0);
}


std::string receiveString(int fd) {
    int len;
    std::string res;
    char c;
    if (recvSth(len, fd) == -1)
        return res;
    for (int i = 0; i < len; ++i) {
        if (recvSth(c, fd) == -1) {
            res.clear();
            return res;
        }
        res.push_back(c);
    }
    return res;
}

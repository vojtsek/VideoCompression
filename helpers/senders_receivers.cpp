#include "headers/include_list.h"
#include "headers/enums_types.h"
#include <fcntl.h>

int sendInt32(int fd, int32_t i) {
    i = htonl(i);
    int w;
    if ((w = write(fd, &i, sizeof (int32_t))) != sizeof(int32_t)) {
        reportDebug("Problem occured while sending the data.", 1);
        return -1;
    }
    return 0;
}

int receiveInt32(int fd, int32_t &i) {
    int32_t ir;
    int r;
     if ((r = read(fd, &ir, sizeof (int32_t))) != sizeof(int32_t)) {
        reportDebug("Problem occured while accepting the data.", 1);
        return (-1);
    }
     i = ntohl(ir);
    return 0;
}

int sendResponse(int fd, RESPONSE_T &resp) {
    int32_t i;
    if (sendInt32(fd, resp) == -1) {
        return -1;
    }
}

int receiveResponse(int fd, RESPONSE_T &resp) {
    int32_t i;
    if (receiveInt32(fd, i) == -1) {
        return -1;
    }
    resp = RESPONSE_T(i);
    return 0;
}

int sendStruct(int fd, const sockaddr_storage &st) {
    MyAddr mad(st);
    if (sendString(fd, mad.addr) == -1) {
        reportDebug("Problem occured while sending the address string.", 1);
        return -1;
    }

    if (sendInt32(fd, mad.port) == -1) {
        reportDebug("Problem occured while sending the port.", 1);
        return -1;
    }

    return 0;
}

int receiveStruct(int fd, struct sockaddr_storage &st) {
    std::string addr_string;
    int32_t port;
    if ((addr_string = receiveString(fd)).empty()) {
        reportDebug("Problem occured while accepting the address string.", 1);
        return -1;
    }

    if (receiveInt32(fd, port) == -1) {
        reportDebug("Problem occured while accepting the address port.", 1);
        return -1;
    }
    st = networkHelper::addr2storage(
                DATA->config.superpeer_addr.c_str(),
                port, AF_INET6);
    return 0;
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
        while ((r = read(fd, buf, DATA->config.getValue("TRANSFER_BUF_LENGTH"))) > 0) {
            received += r;
            if ((w = write(o_file, buf, r)) == -1) {
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
        utilities::rmFile(fn);
        return -1;
    }
    reportDebug("received " + utilities::m_itoa(received), 3);
    close(o_file);
    return 0;
}

int sendFile(int fd, std::string fn) {
    int file;
    off_t fsize, to_sent = 0, r, w;
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

        to_sent = fsize;
        while ((r = read(file, buf, DATA->config.getValue("TRANSFER_BUF_LENGTH"))) > 0) {
            if ((w = write(fd, buf, r)) != -1) {
                to_sent -= w;
            } else {
                reportDebug("Error; sent " + utilities::m_itoa(w), 2);
                throw 1;
            }
        }

        if (to_sent) {
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

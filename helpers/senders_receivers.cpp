#include "headers/include_list.h"
#include "headers/enums_types.h"
#include <fcntl.h>

int64_t sendInt64(int64_t fd, int64_t i) {
    // to network byte order
    i = htonl(i);
    int64_t w;
    // check the size written
    if ((w = write(fd, &i, sizeof (int64_t ))) !=
            sizeof(int64_t )) {
        reportDebug("Problem occured while sending the data.", 1);
        return -1;
    }
    return 0;
}

int64_t receiveInt64(int64_t fd, int64_t &i) {
    int64_t ir;
    int64_t r;
    // should read exactly size of int
     if ((r = read(fd, &ir, sizeof (int64_t ))) !=
             sizeof(int64_t )) {
        reportDebug("Problem occured while accepting int64.", 1);
        return (-1);
    }
     // to host byte order
     i = ntohl(ir);
    return 0;
}

int64_t sendResponse(int64_t fd, RESPONSE_T &resp) {
    // implicit conversion
    if (sendInt64(fd, resp) == -1) {
        return -1;
    }
    return 0;
}

int64_t receiveResponse(int64_t fd, RESPONSE_T &resp) {
    int64_t i;
    if (receiveInt64(fd, i) == -1) {
        return -1;
    }
    // construct the response from int64_t
    resp = RESPONSE_T(i);
    return 0;
}

int64_t sendAdrressStruct(int64_t fd, const sockaddr_storage &st) {
    MyAddr mad(st);
    // send string representation and port separately,
    // will be putted together
    if (sendString(fd, mad.addr) == -1) {
        reportDebug("Problem occured while sending the address string.", 1);
        return -1;
    }

    if (sendInt64(fd, mad.port) == -1) {
        reportDebug("Problem occured while sending the port.", 1);
        return -1;
    }

    return 0;
}

int64_t receiveAddressStruct(int64_t fd, struct sockaddr_storage &st) {
    std::string addr_string;
    int64_t port;
    // receives string representation of the address
    if ((addr_string = receiveString(fd)).empty()) {
        reportDebug("Problem occured while accepting the address string.", 1);
        return -1;
    }

    if (receiveInt64(fd, port) == -1) {
        reportDebug("Problem occured while accepting the address port.", 1);
        return -1;
    }

    // creates the address structure
    int64_t family = AF_INET;
    if (!DATA->config.IPv4_ONLY) {
        family = AF_INET6;
    }
    st = networkHelper::addrstr2storage(
        addr_string.c_str(), port, family);
    return 0;
}

int64_t sendString(int64_t fd, std::string str) {
    // sends the string length
    int64_t len = str.length();
    if (sendInt64(fd, len) == -1) {
        return -1;
    }
    // sends the string char by char
    for (char c : str) {
        if (sendSth(c, fd) == -1) {
            return -1;
        }
    }
    return (0);
}

std::string receiveString(int64_t fd) {
    int64_t len;
    std::string res;
    char c;
    // receive length
    if (receiveInt64(fd, len) == -1) {
        return res;
    }
    // receive the string char by char
    for (int64_t i = 0; i < len; ++i) {
        if (recvSth(c, fd) == -1) {
            res.clear();
            return res;
        }
        res.push_back(c);
    }
    return res;
}

int64_t receiveFile(int64_t fd, std::string fn) {
    int64_t fsize, read_bytes = 0, r, w;
    int64_t o_file;
    char buf[DATA->config.getIntValue("TRANSFER_BUF_LENGTH")];
    try {
        // opens resulting file destructively
        if ((o_file = open(fn.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
            reportDebug(fn + ": Failed to open the output file. ", 2);
            throw 1;
        }

        // receives file size
        if (receiveInt64(fd, fsize) == -1) {
            reportDebug(fn + ": Failed to get file size. ", 2);
            throw 1;
        }
        // reads into the buffer and counts the received bytes
        while ((r = read(fd, buf,
                         DATA->config.getIntValue("TRANSFER_BUF_LENGTH"))) > 0) {
            read_bytes += r;
            // writes read bytes
            if ((w = write(o_file, buf, r)) == -1) {
                reportDebug("Error; received " +
                            utilities::m_itoa(read_bytes), 2);
                throw 1;
            }
        }
        // checks whether sizes corresponds
        if (read_bytes != fsize) {
            reportDebug("Received bytes and expected fsize does not equal. ", 2);
            reportDebug("EXP: " + utilities::m_itoa(fsize), 1);
            reportDebug("REC: " + utilities::m_itoa(read_bytes), 1);
            throw 1;
        }
    } catch (int) {
        reportError("Bad transfer");
        // removes the output
        close(o_file);
        OSHelper::rmFile(fn);
        return -1;
    }
    reportDebug("received " + utilities::m_itoa(read_bytes), 2);
    close(o_file);
    return 0;
}

int64_t sendFile(int64_t fd, std::string fn) {
    int64_t file;
    int64_t fsize, to_sent = 0, r, w;
    char buf[DATA->config.getIntValue("TRANSFER_BUF_LENGTH")];
    try {
        // opens file
        if ((file = open(fn.c_str(), O_RDONLY)) == -1) {
            reportDebug(fn + ": Failed to open.", 2);
            throw 1;
        }

        // gets the size
        if ((fsize = OSHelper::getFileSize(fn)) == -1) {
            reportDebug(fn + ": Failed to get file size", 2);
            throw 1;
        }

        if (sendInt64(fd, fsize) == -1) {
            reportDebug(fn + ": Failed to send file size", 2);
            throw 1;
        }

        // should be zero in the end
        to_sent = fsize;
        // read something to the buffer
        while ((r = read(file, buf,
                         DATA->config.getIntValue("TRANSFER_BUF_LENGTH"))) > 0) {
            // write it to the file descriptor, decrease counter accordingly
            if ((w = write(fd, buf, r)) != -1) {
                to_sent -= w;
            } else {
                reportDebug("Error; sent " + utilities::m_itoa(w), 2);
                throw 1;
            }
        }

        // suppose to be zero
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

int64_t sendCmd(int64_t fd, CMDS cmd) {
    bool response;
    // send command
    if (sendSth(cmd, fd) == -1) {
        reportError("Error send CMD");
        return (-1);
    }

    // receive ack
    // recv Sth should transfer the boolean properly
    if (recvSth(response, fd) == -1) {
        reportError("ERROR conf CMD");
        return (-1);
    }

    // response has to be true
    if (!response) {
        reportDebug("The command was not accepted.", 1);
        return (-1);
    }
    return (0);
}

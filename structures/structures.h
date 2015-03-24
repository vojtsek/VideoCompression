#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <arpa/inet.h>

/*! \file structures.h
 * \brief contains definitions of some helping structures
 */

/*! \class NetworkHandler
 * \brief externaly defined class */
class NetworkHandler;

/*! \struct FileInfo
 * \brief stores information about loaded file
 */

struct FileInfo {
    std::string fpath, codec, extension, basename, format;
    double fsize, duration, bitrate;
};

/*! \struct Listener
 * \brief abstract class to be used to deriving events
 * abstract struct which defines methods
 * \a invoke, \a getHash and \a equalsTo
 */

struct Listener {
    virtual void invoke(NetworkHandler &handler) = 0;
    virtual std::string getHash() = 0;
    virtual bool equalsTo(Listener *that) = 0;
};

/*! \struct Sendable
 * \brief abstract class whose descendants can be sent over the network
 */

struct Sendable {
    virtual int32_t send(int32_t fd) = 0;
    virtual int32_t receive(int32_t fd) = 0;
};

/*! \struct MyAddr
 * \brief structure that helps to maintain network adresses
 * it is constructed from struct sockaddr_storage
 * and extract information about adress and port
 */

struct MyAddr {
    std::string addr;
    int32_t port;
    void print();
    std::string get();
    //bool equalsTo(const struct sockaddr_storage &that);
    MyAddr(const struct sockaddr_storage &addr);
};

#endif // STRUCTURES_H

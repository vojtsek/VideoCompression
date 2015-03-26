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
 * \brief abstract struct to be used to deriving events
 */

struct Listener {

    /*! \fn invoke
     * a function invoked when an event occurs
     */
    virtual void invoke(NetworkHandler &handler) = 0;

    /*! \fn getHash
     * get string representation of object
     */
    virtual std::string getHash() = 0;

    /*! \fn equalsTo
     * decide, whether the provided object is equal to this
     */
    virtual bool equalsTo(Listener *that) = 0;
};

/*! \struct Sendable
 * \brief abstract class whose descendants can be sent over the network
 */

struct Sendable {
    /*! \fn send
     * Sends its content over the provided file descriptor.
     * It should be received by \a receive method.
     */
    virtual int32_t send(int32_t fd) = 0;

    /*! \fn receive
     * Receives content of the structure \a sent by send method.
     */
    virtual int32_t receive(int32_t fd) = 0;
};

/*! \struct MyAddr
 * \brief structure that helps to maintain network adresses
 * it is constructed from struct sockaddr_storage
 * and extract information about adress and port
 * \param addr
 * string representation of the network adress
 * \param port
 * numeric representation of the port
 */

struct MyAddr : public Sendable {
    std::string addr;
    int32_t port, family, TTL;
    void print();
    std::string get();
    struct sockaddr_storage getAddress();
    virtual int32_t send(int32_t fd);
    virtual int32_t receive(int32_t fd);
    //bool equalsTo(const struct sockaddr_storage &that);
    MyAddr(const struct sockaddr_storage &addr);
    MyAddr() {}
};

#endif // STRUCTURES_H

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
    //! path to the file
    std::string fpath;
    //! coding of the file's video stream
    std::string codec;
    //! file extension
    std::string extension;
    //! basename of the file
    std::string basename;
    //! format of the container
    std::string format;
    //! file size in
    int64_t fsize;
    //! duration of the video
    int64_t duration;
    //! bitrate of the video
    int64_t bitrate;
};

/*! \struct Listener
 * \brief abstract struct to be used to deriving events
 */

struct Listener {

    /*! \fn invoke
     * a function invoked when an event occurs
     */
    virtual void invoke(NetworkHandler &handler) = 0;

    /*! \fn toString
     * get string representation of object
     */
    virtual std::string toString() = 0;
};

/*! \struct Sendable
 * \brief abstract class whose descendants can be sent over the network
 */

struct Sendable {
    /*! \fn send
     * Sends its content over the provided file descriptor.
     * It should be received by \a receive method.
     */
    virtual int64_t send(int64_t fd) = 0;

    /*! \fn receive
     * Receives content of the structure \a sent by send method.
     */
    virtual int64_t receive(int64_t fd) = 0;
};

/*! \struct MyAddr
 * \brief structure that helps to maintain network adresses
 * it is constructed from struct sockaddr_storage
 * and extract information about adress and port
 */

struct MyAddr : public Sendable {
        //! string representation of the network adress
    std::string addr;
        //! numeric representation of the port
    int64_t port;
    //! address family
    int64_t family;
    //! time to live, used when gathering neighbors
    int64_t TTL;

    /*!
     * \brief print prints the string representation of the address
     */
    void print();

    /*!
     * \brief get gets string representatin of the address structure
     * \return string consisting of address and port
     */
    std::string get();

    /*!
     * \brief getAddress gets raw address structure
     * \return struct sockaddr_storage
     */
    struct sockaddr_storage getAddress();

    /*!
     * \brief send allows to correctly transfer over the network
     * \param fd file descriptor of the connection
     * \return zero on success
     * Transfers field by field.
     */
    virtual int64_t send(int64_t fd);

    /*!
     * \brief receive allows to correctly transfer over the network
     * \param fd file descriptor of the connection
     * \return zero on success
     * Transfers field by field.
     */
    virtual int64_t receive(int64_t fd);
    MyAddr(const struct sockaddr_storage &addr);
    MyAddr() {}
    virtual ~MyAddr() {}
};

#endif // STRUCTURES_H

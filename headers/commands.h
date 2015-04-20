#include "structures/NetworkHandler.h"

#include <string>
#include <iostream>
#include <sstream>

#ifndef COMMANDS_H
#define COMMANDS_H


class VideoState;

/*!
 * \brief The Command class
 * Ancestor class for all commands
 * Commands are stored in the global map and are invoked on demand.
 */
class Command {
protected:
    /*!
     * \brief state
     * pointer to the VideoState instance
     */
    VideoState *state;
public:
    Command(VideoState *st) {
        state = st;
    }
    /*!
     * \brief execute
     * essential method which executes the desired action
     */
    virtual void execute() = 0;
    virtual ~Command() {}
};

/*!
 * \brief The NetworkCommand class
 * Command subclass, allows to connect to the remote adress
 * Designed to comunicate via network sockets, altough it is
 * not mandatory.
 * Basically each descendant has its counterpart and together they
 * care about the logic of the command.
 */
class NetworkCommand: public Command {
protected:
    //! pointer to NetworkHandlert instance
    NetworkHandler *handler;
public:
    NetworkCommand(VideoState *state, NetworkHandler *nhandler):
        Command(state), handler(nhandler) {}
    /*!
     * \brief getName helper function to identify command
     * \return string identifying the command
     */
    virtual std::string getName() {
        return(typeid(this).name());
    }

    /*!
     * \brief execute method implementic logic,
     *  in case of no connection actually needed
     */
    virtual void execute() {}

    /*!
     * \brief execute implements logic
     * \param fd file descriptor of the connection
     * \param addr structure containing address of the peer
     * \param data optionally passed pointer to data
     * \return true on success
     */
    virtual bool execute(int64_t fd, struct sockaddr_storage &addr, void *data) = 0;
};

#endif

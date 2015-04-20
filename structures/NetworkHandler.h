#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H
#include "headers/defines.h"
#include "structures/NeighborInfo.h"

#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <curses.h>
#include <sys/socket.h>

/*!
 * \brief The NetworkHandler struct
 * provides several methods to handle network
 * related stuffs
 * also holds adresses of potential neighbors
 */
struct NetworkHandler {
    //! potential neighbors - addresses to contact
    std::vector<struct sockaddr_storage> potential_neighbors;
    //! to synchronize access to potential neighbors
    std::mutex potential_mtx;


    /*!
     * \brief spawnOutgoingConnection spawns communication
     *  with the given address
     * \param addr address with which it should communicate
     * \param fd file descriptor associated with the connection
     * \param cmds vector of commands that should be processed
     * \param async whether the connection should be asynchronous
     * \param data pointer to data commands are supposed to access
     * The method is called when a connection with some remote peer
     * is created, handles sending the commands provided,
     * ensures correct communication with the other side.
     * Each command runs and its result determines what next.
     * The controlling routine is spawned in the separate thread, if async,
     * this thread is then detached.
		 * Finally, the fd is closed.
     */
    void spawnOutgoingConnection(struct sockaddr_storage addr, int64_t fd,
                                 std::vector<CMDS> cmds, bool async, void *data);
		/*!
		* \brief handles incoming communication
		* \param addr address with which it should communicate
		* \param fd file descriptor associated with the connection
		* \param async whether the connection should be asynchronous
		* The method is called, given a file descriptor obtained from accepted connection
		* Waits for CMDS type to be received and tries to map it to a command
		* and execute it. Fails otherwise.
		* Runs in separate thread, closes the fd eventually.
		*/
    void spawnIncomingConnection(struct sockaddr_storage addr, int64_t fd, bool async);

		/*!
		* \brief start_listening starts listening on the given port
		* \param port port number with which should be the listening socket bound
		* \return -1 on failure
		* Creates the socket, binds it and starts listening on the given port.
		* Incoming connection are handled using spawnIncomingConnection call.
		*/
    int64_t start_listening(int64_t port);

		/*!
		* \brief checkNeighbor checks status of the given neighbor
		* \param addr address of the neighbor to be checked
		* \return on success, file descriptor of the connection, -1 otherwise
		* Tries to connect to the neighbor.
		* If fails, removes the neighbor, otherwise
		* updates neighbor information and returns
		* the file descriptor associated with the connection.
		*/
    int64_t checkNeighbor(struct sockaddr_storage addr);

		/*!
		* \brief gets count of potential neighbors
		* \return count of pot. neighbors
		*/
    int64_t getPotentialNeighborsCount();

    /*!
     * \brief connectPeer makes connection to the desired peer
     * \param addr network address structure to connect to.
     * \return file descriptor of the connection
     */
    int64_t connectPeer(struct sockaddr_storage *addr);

		/*!
		* \brief confirmNeighbor connect to a remote peer and spawns Confirm command
		* \param addr address of the neighbor to be confirmed
		*/
    void confirmNeighbor(struct sockaddr_storage addr);

		/*!
		* \brief obtainNeighbors tries to gain more neighbors to the list
		* If potential neighbor is available, tries to confirm him,
		* othewise first tries to gain some potential neighbors
		* using collectNeighbors
		*/
    void obtainNeighbors();

		/*!
		* \brief gatherNeighbors spreads the request message
		* \param TTL how many hops to make
		* \param requester_addr address of the node which is requesting
		* \param ngh_addr address to send, next hop
		* handles sending the request to next neighbor,
		* however ain logic in the CmdGather*
		*/
    void gatherNeighbors(int64_t TTL,
            const struct sockaddr_storage &requester_addr,
            const struct sockaddr_storage &ngh_addr);

		/*!
		* \brief collectNeighbors tries to collect more potential neighbors
		* contacts neighbor, if any, or superpeer
		* asks for new adresses
		* different from gather neighbors
		*/
    void collectNeighbors();

		/*!
		* \brief pings the super peer
		* Contacts the superpeer and pings it
		* invoked periodically
		*/
    void contactSuperPeer();

		/*!
		* \brief askForAddresses sends a request for new neighbors
		* \param addr address of the neighbor which should be asked
		* Makes a connection and invokes the CmdAsk command,
		* which should obtain new addresses.
		*/
    void askForAddresses(const struct sockaddr_storage &addr);

		/*!
		* \brief addNewNeighbor adds new neighbors to the list
		* \param potential whether the neighbor is potential
		* \param addr address of the neighbors
		* adds the given neighbor to a list in NeighborStorage
		* or adds it to the potential neighbors.
		*/
    void addNewNeighbor(bool potential,const struct sockaddr_storage &addr);
};

#endif // NETWORKHANDLER_H

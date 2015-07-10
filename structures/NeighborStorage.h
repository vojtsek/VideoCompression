#ifndef NEIGHBORSTORAGE_H
#define NEIGHBORSTORAGE_H
#include <vector>
#include <functional>
#include <mutex>
#include <arpa/inet.h>
#include "headers/enums_types.h"

struct NeighborInfo;
struct TransferInfo;
struct NetworkHandler;

/*!
 * \brief The NeighborStorage class maintains the neighbors
 * Provides interface to work with the list of neighbors;
 * The neighbors are stored in NeighborInfo structures
 * Synchronization is granted.
 */
class NeighborStorage
{
    //! the map of neighbors, they are mapped to their string representations
    neighbor_storageT neighbors;
    //! vector of free neighbors
    std::vector<NeighborInfo *> free_neighbors;
    //! potential neighbors
    std::vector<struct sockaddr_storage> potential_neighbors;
    //! mutex to synchronize access to neighbor list
    std::mutex n_mtx;

    /*!
     * \brief _contains determines whether neighbors list contains given address
     * \param addr address to match
     * \return true if neighbor is present
     * Does not lock.
     */
    bool _contains(const struct sockaddr_storage &addr);

    /*!
     * \brief _get gets the info about neighbor with the adddress, does not lock
     * \param addr address of the neighbor
     * \return neighbor with given address
     */
    NeighborInfo *_get(const struct sockaddr_storage &addr);
    /*!
     * \brief createHash gets string representation corresponding to given neighbor
     * \param addr address of desired neighbor
     * \return unique string id
     */
    std::string _createHash(const struct sockaddr_storage &addr);

    /*!
     * \brief trashNeighborChunks gets rid of chunks associated with given neighbor
     * \param addr address of neighbor whose chunks should be removed
     */
    void trashNeighborChunks(const struct sockaddr_storage &addr);

public:

    /*!
     * \brief getNeighborCount gets current count of members
     * \return count of members
     */
    int64_t getNeighborCount();

    /*!
     * \brief getBiggestDifference computes biggest difference between processed chunks
     * \return the biggest difference
     */
    int64_t getBiggestDifference();

    /*!
     * \brief getNeighborInfo returns NeighborInfo of neighbor
     * with corresponding address
     * \param addr address to match
     * \return pointer to corresponding NeighborInfo, nullptr on failure
     */
    NeighborInfo *getNeighborInfo(
            const struct sockaddr_storage &addr);

    /*!
     * \brief getFreeNeighbor sorts the free neighbors
     * and returns the first one
     * \param addr address structure to load neighbor address
     * \return one on success
     */
    int64_t getFreeNeighbor(struct sockaddr_storage &addr);

    /*!
     * \brief getRandomNeighbor gets random neighbor
     * \param addr address structure to load neighbor address into
     * \return one on success
     */
    int64_t getRandomNeighbor(struct sockaddr_storage &addr);

    /*!
     * \brief getNeighborAdresses gets vector of addresses
     * of desired count of neighbors
     * \param count how many addresses get
     * \return vector of addresses
     */
    std::vector<struct sockaddr_storage> getNeighborAdresses(int64_t count);

    /*!
     * \brief removeNeighbor removes given neighbor and
     * handles connected stuff
     * \param addr sockaddr_storage containing neighbor to remove
     * \return one, if neighbor was removed, zero otherwise
     */
    int64_t removeNeighbor(const struct sockaddr_storage &addr);

    /*!
     * \brief removeDirty traverses the list and remove dirty neighbors
     * i.e. did not manage to contact them while periodically checking
     */
    void removeDirty();

    /*!
     * \brief printNeighborsInfo prints information about neighbors
     * to info window
     */
    void printNeighborsInfo();

    /*!
     * \brief addNewNeighbor adds the new neighbor to the list
     * \param addr sockaddr_storage containing the address of the new neighbor
     * also pushes it as the periodic listener
     */
    void addNewNeighbor(const struct sockaddr_storage &addr);

    /*!
     * \brief setInterval updates the neighbor time to check
     * \param addr address of the updated neighbor
     * \param i what number to set
     */
    void setInterval(const struct sockaddr_storage &addr, int64_t i);

    /*!
     * \brief updateQuality increases the given neighbor
     * quality by given value
     * \param addr address of neighbor to update
     * \param q difference of quality
     */
    void updateQuality(const struct sockaddr_storage &addr, int64_t q);

    /*!
     * \brief setNeighborFree changes the neighbor's free state according to parameter
     * \param addr address of the neighbor to change
     * \param free value that should be set
     */
    void setNeighborFree(const struct sockaddr_storage &addr, bool free);

    /*!
     * \brief assignChunk associates chunk with the neighbor
     * \param addr neighbor to assign to
     * \param assign whether assign or delete
     * \param ti chunk to assign
     */
    void assignChunk(const struct sockaddr_storage &addr,
                     bool assign, TransferInfo *ti);

    /*!
     * \brief removeNeighborChunk unassigns chunk from the given neighbor
     * \param ngh neighbor to unassign from
     * \param ti chunk to be unassigned
     */
    void removeNeighborChunk(NeighborInfo *ngh, TransferInfo *ti);
    /*!
     * \brief applyToNeighbors traverses the list of neighbors
     * and applies given function to each
     * \param func function to apply
     */
    void applyToNeighbors(
            std::function<void (std::pair<std::string, NeighborInfo *>)> func);

    void cancelChunk(TransferInfo *ti, NetworkHandler *handler);
    /*!
     * \brief clear clears the storage
     */
    void clear();

        /*!
     * \brief contains determines whether neighbors list contains given address
     * \param addr address to match
     * \return true if neighbor is present
     * Just wrapper to synchronize _contains call
     */
    bool contains(const struct sockaddr_storage &addr);

    NeighborStorage();
};

#endif // NEIGHBORSTORAGE_H

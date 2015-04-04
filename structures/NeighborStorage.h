#ifndef NEIGHBORSTORAGE_H
#define NEIGHBORSTORAGE_H
#include <vector>
#include <functional>
#include <mutex>
#include <arpa/inet.h>
#include "headers/enums_types.h"

struct NeighborInfo;

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
public:

    /*!
     * \brief getNeighborCount gets current count of members
     * \return count of members
     */
    int32_t getNeighborCount();

    /*!
     * \brief getNeighborInfo returns NeighborInfo of neighbor
     * with corresponding address
     * \param addr address to match
     * \param lock whether lock or not
     * \return pointer to corresponding NeighborInfo, nullptr on failure
     */
    NeighborInfo *getNeighborInfo(
            const struct sockaddr_storage &addr, bool lock);

    /*!
     * \brief getFreeNeighbor sorts the free neighbors
     * and returns the first one
     * \param addr address structure to load neighbor address
     * \return one on success
     */
    int32_t getFreeNeighbor(struct sockaddr_storage &addr);

    /*!
     * \brief getRandomNeighbor gets random neighbor
     * \param addr address structure to load neighbor address into
     * \return one on success
     */
    int32_t getRandomNeighbor(struct sockaddr_storage &addr);

    /*!
     * \brief getNeighborAdresses gets vector of addresses
     * of desired count of neighbors
     * \param count how many addresses get
     * \return vector of addresses
     */
    std::vector<struct sockaddr_storage> getNeighborAdresses(int32_t count);

    /*!
     * \brief removeNeighbor removes given neighbor and
     * handles connected stuff
     * \param addr sockaddr_storage containing neighbor to remove
     * \return one, if neighbor was removed, zero otherwise
     */
    int32_t removeNeighbor(const struct sockaddr_storage &addr);

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
    void setInterval(const struct sockaddr_storage &addr, int32_t i);

    /*!
     * \brief updateQuality increases the given neighbor
     * quality by given value
     * \param addr address of neighbor to update
     * \param q difference of quality
     */
    void updateQuality(const struct sockaddr_storage &addr, int32_t q);

    /*!
     * \brief setNeighborFree changes the neighbor's free state according to parameter
     * \param addr address of the neighbor to change
     * \param free value that should be set
     */
    void setNeighborFree(const struct sockaddr_storage &addr, bool free);

    /*!
     * \brief applyToNeighbors traverses the list of neighbors
     * and applies given function to each
     * \param func function to apply
     */
    void applyToNeighbors(
            std::function<void (std::pair<std::string, NeighborInfo *>)> func);
    NeighborStorage();
};

#endif // NEIGHBORSTORAGE_H

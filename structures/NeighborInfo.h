#ifndef NEIGHBORINFO_H
#define NEIGHBORINFO_H

#include "structures/structures.h"
#include "headers/defines.h"

/*!
 * \brief The NeighborInfo struct
 * holds information about particular neighbor
 */
struct NeighborInfo : public Listener {
    //! address and port of the neighbor
    struct sockaddr_storage address;
    //! how many intervals before check
    // decreased periodically, reset each time the neighbor is checked
    int64_t intervals;
    //! represents quality - average computation time
    int64_t quality;
    //! overall computation time
    int64_t overall_time;
    //! number of chunks processed by this neighbor
    int64_t processed_chunks;
    //! whether the neighbor is free
    bool free;
    //! it's going to be remove
    bool dirty;

    /*!
     * \brief NeighborInfo::getInfo returns brief neighbor information as a string
     * \return neighbor information stringified
     */
    std::string getInfo();

    /*!
     * \brief NeighborInfo::printInfo prints information about neighbor
     */
    void printInfo();

    /*!
     * \brief NeighborInfo::invoke checks the neighbor state
     * \param net_handler
     * is supposed to be invoked periodically;
     * decreases counter and when zero, checks the neighbor state
     */
    virtual void invoke(NetworkHandler &handler);

    /*!
     * \brief NeighborInfo::toString return unique string representation
     * \return string representation
     */
    virtual std::string toString();
    virtual ~NeighborInfo() {}

    NeighborInfo(const struct sockaddr_storage &addr):
        quality(0), overall_time(0), processed_chunks(0),
        free(true), dirty(false) {
        address = addr;
    }
};

#endif // NEIGHBORINFO_H

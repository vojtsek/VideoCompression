#ifndef NEIGHBORINFO_H
#define NEIGHBORINFO_H

#include "structures/structures.h"
#include "headers/defines.h"

struct TransferInfo;
/*!
 * \brief The NeighborInfo struct
 * holds information about particular neighbor
 */
struct NeighborInfo : public Listener {
    //! address and port of the neighbor
    struct sockaddr_storage address;
    //! chunks associated with the neighbor
    std::vector<TransferInfo *> chunks_assigned;
    //! how many intervals before check
    // decreased periodically, reset each time the neighbor is checked
    int64_t intervals;
    //! represents quality - average computation time
    double quality;
    //! overall computation time
    double overall_time;
    //! number of chunks processed by this neighbor
    int64_t processed_chunks;
    //! whether the neighbor is free
    bool free;
    //! it's going to be removed
    bool dirty;

    /*!
     * \brief NeighborInfo::getInfo returns brief neighbor information as a string
     * \return neighbor information stringified
     */
    std::string getInfo();

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

    explicit NeighborInfo(const struct sockaddr_storage &addr):
        intervals(0), quality(0), overall_time(0), processed_chunks(0),
        free(true), dirty(false) {
        address = addr;
    }
};

#endif // NEIGHBORINFO_H

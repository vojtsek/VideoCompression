#ifndef NEIGHBORINFO_H
#define NEIGHBORINFO_H

#include "structures/structures.h"
#include "headers/defines.h"

/*!
 * \brief The NeighborInfo struct
 * holds information about particular neighbor
 */
struct NeighborInfo : public Listener {
    struct sockaddr_storage address;
    int32_t intervals;
    int32_t quality;
    int32_t overall_time;
    int32_t processed_chunks;
    bool free;
    bool dirty;

    std::string getInfo();
    void printInfo();
    virtual bool equalsTo(Listener *that);
    virtual void invoke(NetworkHandler &handler);
    virtual std::string toString();
    virtual ~NeighborInfo() {}

    NeighborInfo(const struct sockaddr_storage &addr):
        quality(0), overall_time(0), processed_chunks(0),
        free(true), dirty(false) {
        address = addr;
    }
};

#endif // NEIGHBORINFO_H

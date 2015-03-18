#ifndef NEIGHBORINFO_H
#define NEIGHBORINFO_H

#include "structures/structures.h"
struct NeighborInfo : public Listener {
    struct sockaddr_storage address;
    int intervals;
    int quality;
    int overall_time;
    int processed_chunks;
    bool confirmed;
    bool active;
    bool free;

    void printInfo();
    virtual bool equalsTo(Listener *that);
    virtual void invoke(NetworkHandler &handler);
    virtual std::string getHash();
    virtual ~NeighborInfo() {}

    NeighborInfo(struct sockaddr_storage &addr):
        intervals(DATA->config.getValue("CHECK_INTERVALS")),
        quality(0), free(true) {
        address = addr;
    }
};

/*
template<typename T>
struct StructureRef {
    T *data;
    bool valid;
    T &operator->()
};
*/

#endif // NEIGHBORINFO_H

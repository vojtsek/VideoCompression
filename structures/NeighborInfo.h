#ifndef NEIGHBORINFO_H
#define NEIGHBORINFO_H

#include "structures/structures.h"
#include "headers/defines.h"

struct NeighborInfo : public Listener {
    struct sockaddr_storage address;
    int32_t intervals;
    int32_t quality;
    int32_t overall_time;
    int32_t processed_chunks;
    bool confirmed;
    bool active;
    bool free;

    void printInfo();
    virtual bool equalsTo(Listener *that);
    virtual void invoke(NetworkHandler &handler);
    virtual std::string getHash();
    virtual ~NeighborInfo() {}

    NeighborInfo(const struct sockaddr_storage &addr):
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

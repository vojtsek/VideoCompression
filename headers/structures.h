#ifndef STRUCTURES_H
#define STRUCTURES_H


/**
 * @brief The Listener struct
 *
 * Is able to react on events, via the invoke() method call.
 */
struct Listener {
    virtual void invoke(NetworkHandler &handler) = 0;
    virtual std::string getHash() = 0;
};

struct Sendable {
    virtual int32_t send(int32_t fd) = 0;
    virtual int32_t receive(int32_t fd) = 0;
};


#endif // STRUCTURES_H

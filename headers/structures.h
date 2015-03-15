#ifndef STRUCTURES_H
#define STRUCTURES_H


struct Listener {
    virtual void invoke(NetworkHandler &handler) = 0;
    virtual std::string getHash() = 0;
};

struct Sendable {
    virtual int send(int fd) = 0;
    virtual int receive(int fd) = 0;
};


#endif // STRUCTURES_H

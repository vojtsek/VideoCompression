#ifndef STRUCTURES_H
#define STRUCTURES_H

class NetworkHandler;

struct FileInfo {
    std::string fpath, codec, extension, basename, format;
    double fsize, duration, bitrate;
};


struct Listener {
    virtual void invoke(NetworkHandler &handler) = 0;
    virtual std::string getHash() = 0;
    virtual bool equalsTo(Listener *that) = 0;
};

struct Sendable {
    virtual int send(int fd) = 0;
    virtual int receive(int fd) = 0;
};

struct MyAddr {
    std::string addr;
    int port;
    void print();
    std::string get();
    //bool equalsTo(const struct sockaddr_storage &that);
    MyAddr(struct sockaddr_storage &addr);
};

#endif // STRUCTURES_H

#ifndef TRANSFERINFO_H
#define TRANSFERINFO_H

#include "structures/structures.h"

struct TransferInfo : public Listener, Sendable {
    bool addressed;
    int chunk_size, time_left, tries_left;
    struct sockaddr_storage address, src_address;
    std::string job_id;
    std::string name; // without extension
    std::string original_extension;
    std::string desired_extension;
    std::string path;
    std::string output_codec;
    std::string timestamp;

    virtual void invoke(NetworkHandler &handler);
    virtual std::string getHash();
    virtual int send(int fd);
    virtual int receive(int fd);
    void print();

    TransferInfo():addressed(false) {};

    TransferInfo(struct sockaddr_storage addr, int size,
                 std::string ji, std::string n, std::string oe, std::string de,
                 std::string p, std::string oc): addressed(true), chunk_size(size),
        time_left(DATA->config.intValues.at("COMPUTATION_TIMEOUT")),
        tries_left(DATA->config.intValues.at("TRIES_BEFORE_RESEND")),
        job_id(ji), name(n), original_extension(oe), desired_extension(de),
        path(p), output_codec(oc), timestamp(utilities::getTimestamp()) {
        src_address = addr;
    }

    TransferInfo(int size,
                 std::string ji, std::string n, std::string oe, std::string de,
                 std::string p, std::string oc): addressed(false), chunk_size(size),
        time_left(DATA->config.intValues.at("COMPUTATION_TIMEOUT")),
        tries_left(DATA->config.intValues.at("TRIES_BEFORE_RESEND")),
        job_id(ji), name(n), original_extension(oe), desired_extension(de),
        path(p), output_codec(oc), timestamp(utilities::getTimestamp()) {}

    virtual ~TransferInfo() {}
};


#endif // TRANSFERINFO_H

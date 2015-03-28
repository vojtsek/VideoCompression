#ifndef TRANSFERINFO_H
#define TRANSFERINFO_H

#include "structures/structures.h"
#include "headers/utilities.h"

struct TransferInfo : public Listener, Sendable {
    bool addressed;
    int32_t chunk_size, time_left, tries_left,
    sending_time, receiving_time,
    sent_times, encoding_time;
    struct sockaddr_storage address, src_address;
    std::string job_id;
    std::string name; // without extension
    std::string original_extension;
    std::string desired_extension;
    std::string path;
    std::string output_codec;
    std::string timestamp;
    std::string getInfo();
    std::string getCSV();

    virtual void invoke(NetworkHandler &handler);
    virtual std::string toString();
    virtual int32_t send(int32_t fd);
    virtual int32_t receive(int32_t fd);
    void print();
    virtual bool equalsTo(Listener *that);

    TransferInfo(): addressed(false) {}

    TransferInfo(struct sockaddr_storage addr, int32_t size,
                 std::string ji, std::string n, std::string oe, std::string de,
                 std::string p, std::string oc): addressed(true), chunk_size(size),
        time_left(DATA->config.intValues.at("COMPUTATION_TIMEOUT")),
        tries_left(DATA->config.intValues.at("TRIES_BEFORE_RESEND")),
        sending_time(0), receiving_time(0),
        sent_times(0), encoding_time(0),
        job_id(ji), name(n), original_extension(oe), desired_extension(de),
        path(p), output_codec(oc), timestamp(utilities::getTimestamp()) {
        src_address = addr;
    }

    TransferInfo(int32_t size,
                 std::string ji, std::string n, std::string oe, std::string de,
                 std::string p, std::string oc): addressed(false), chunk_size(size),
        time_left(DATA->config.intValues.at("COMPUTATION_TIMEOUT")),
        tries_left(DATA->config.intValues.at("TRIES_BEFORE_RESEND")),
        sending_time(0), receiving_time(0),
        sent_times(0), encoding_time(0),
        job_id(ji), name(n), original_extension(oe), desired_extension(de),
        path(p), output_codec(oc), timestamp(utilities::getTimestamp()) {}

    virtual ~TransferInfo() {}
};


#endif // TRANSFERINFO_H

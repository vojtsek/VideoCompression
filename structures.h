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

struct State {
    bool enough_neighbors = false;
    bool working = false;
    std::atomic<int> can_accept, to_send, to_recv;
};


struct IO_Data {
    IO_Data(): info_handler(WindowPrinter::STATIC, false, WindowPrinter::TOP),
        status_handler(WindowPrinter::UP, true, WindowPrinter::BOTTOM) {}
    WindowPrinter info_handler, status_handler;
    int status_y = 0, perc_y = 0, question_y = 0;
    WINDOW *status_win, *info_win;
};

struct Mutexes_Data {
    std::mutex I_mtx, O_mtx, report_mtx, chunk_mtx, send_mtx;
    std::condition_variable IO_cond, chunk_cond, send_cond;
    bool using_I = false, using_O = false, process_deq_used = false, send_deq_used = false;
};

struct Configuration {
    bool is_superpeer = false, IPv4_ONLY;
    std::map<std::string, int> intValues;
    std::string working_dir, superpeer_addr;
    int getValue(std::string key);
};

struct TransferInfo;
struct Data {
    static std::shared_ptr<Data> getInstance() {
        if(!inst) {
            inst = std::shared_ptr<Data>(new Data);
        }
        return inst;
    }

    cmd_storage_t cmds;
    net_cmd_storage_t net_cmds;
    IO_Data io_data;
    Mutexes_Data m_data;
    Configuration config;
    State state;
    std::unordered_map<std::string, TransferInfo *> waiting_chunks;
    std::unordered_map<std::string, Listener *> periodic_listeners; //TODO: hashmap, so easy deletion?
    std::unordered_map<std::string, TransferInfo *> chunks_received; //TODO: hashmap, so easy deletion?
    std::deque<TransferInfo *> chunks_to_encode;
    std::deque<TransferInfo *> chunks_to_send;
    static std::vector<std::string> getKnownCodecs() {
        return {"libx264", "msmpeg"};
    }
    static std::vector<std::string> getKnownFormats() {
        return {"avi", "mkv", "ogg"};
    }
    static std::map<wchar_t, CMDS> getCmdMapping() {
        return {
            { KEY_F(6), SHOW },
            { KEY_F(7), START },
            { KEY_F(8), LOAD },
            { KEY_F(9), SET } };
    }

private:
    static std::shared_ptr<Data> inst;
};


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
    virtual void invoke(NetworkHandler &handler);
    virtual std::string getHash();
    virtual ~NeighborInfo() {}

    NeighborInfo(struct sockaddr_storage &addr):
        intervals(DATA->config.getValue("CHECK_INTERVALS")),
        quality(0), free(true) {
        address = addr;
    }
};

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


#endif // STRUCTURES_H

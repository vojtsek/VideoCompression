#ifndef SINGLETONS_H
#define SINGLETONS_H

#include "enums_types.h"
struct TransferInfo;
struct Listener;

struct State {
    bool enough_neighbors = false;
    bool working = false;
    std::atomic<int> can_accept, to_send, to_recv;
};


struct IO_Data {
    IO_Data(): info_handler(STATIC, false, TOP),
        status_handler(UP, true, BOTTOM) {}
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


#endif // SINGLETONS_H

#ifndef SINGLETONS_H
#define SINGLETONS_H

#include <atomic>
#include <unordered_map>
#include "structures/WindowPrinter.h"
#include "headers/enums_types.h"
#include "structures/SynchronizedQueue.h"
#include "structures/NeighborStorage.h"

struct TransferInfo;
struct Listener;

struct State {
    bool enough_neighbors = false;
    bool working = false;
    std::atomic<int> can_accept, to_recv;
};


struct IO_Data {
    IO_Data(std::string log_location): info_handler(STATIC, false, TOP, log_location),
        status_handler(UP, true, BOTTOM, log_location) {}
    WindowPrinter info_handler, status_handler;
    int32_t status_y = 0, perc_y = 0, question_y = 0;
    WINDOW *status_win, *info_win;
    void changeLogLocation(std::string log_location) {
        status_handler.changeLogLocation(log_location);
    }
};

struct Mutexes_Data {
    std::mutex I_mtx, O_mtx, report_mtx, chunk_mtx, send_mtx;
    std::condition_variable IO_cond, chunk_cond, send_cond;
    bool using_I = false, using_O = false, process_deq_used = false, send_deq_used = false;
};

struct Configuration {
    bool is_superpeer = false,
    IPv4_ONLY, serve_while_working;
    int32_t debug_level = 0;
    std::map<std::string, int32_t> intValues;
    std::map<std::string, std::string> strValues;
    std::string working_dir, superpeer_addr;
    int32_t getIntValue(std::string key);
    std::string getStringValue(std::string key);
};

struct Data {
    static std::shared_ptr<Data> getInstance(std::string log_location) {
        if(!inst) {
            inst = std::shared_ptr<Data>(new Data(log_location));
        }
        return inst;
    }

    cmd_storage_t cmds;
    net_cmd_storage_t net_cmds;
    IO_Data io_data;
    Mutexes_Data m_data;
    Configuration config;
    State state;
    NeighborStorage neighbors;
    SynchronizedQueue<TransferInfo> chunks_to_send, chunks_to_encode;
    SynchronizedMap<TransferInfo> chunks_received, chunks_returned,
    chunks_to_process;
    SynchronizedMap<Listener> periodic_listeners;
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
            { KEY_F(9), SET },
            { KEY_UP, SCROLL_UP },
            { KEY_DOWN, SCROLL_DOWN }
        };
    }
    Data(std::string log_location) : io_data(log_location) {}

private:
    static std::shared_ptr<Data> inst;
};


#endif // SINGLETONS_H

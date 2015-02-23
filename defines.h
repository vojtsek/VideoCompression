#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include <memory>
#include <chrono>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <curses.h>

#ifndef DEFINES_H
#define DEFINES_H
#define DEBUG_LEVEL 1
#define STATUS_LENGTH 10
#define CHUNK_SIZE 40048576
#define INFO_LINES 15
#define LINE_LENGTH 80
#define MIN_NEIGHBOR_COUNT 4
#define LISTENING_PORT 25000
#define SUPERPEER_PORT 26000
#define SUPERPEER_ADDR "127.0.0.1"
#define CHECK_INTERVALS 20
#define BUF_LENGTH 256
#define DEFAULT 1
#define RED 2
#define GREEN 3
#define GREY 4
#define BLUE 5
#define YELLOWALL 6
#define GREYALL 7
#define CYANALL 8
#define INVERTED 9
#define COLOR_GREY 100
#define BG 20
#define BG_COL COLOR_BLACK
#define ST_Y 10
#define DATA Data::getInstance()
#define show(x, y) wprintw(DATA->io_data.info_win, "%15s%35s\n", x, y);

class NetworkHandle;
template <typename Measure_inT = std::chrono::milliseconds>
struct Measured {
    template <typename FuncT, typename ... args>
    static typename Measure_inT::rep exec_measure(FuncT func, args && ... arguments) {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        func(std::forward<args>(arguments) ...);
        Measure_inT duration = std::chrono::duration_cast<Measure_inT>
                                    (std::chrono::system_clock::now() - start);
        return duration.count();
    }
};

namespace common {
    std::string getTimestamp();
}

typedef struct FileInfo {
    std::string fpath, codec, extension, basename, format;
    double fsize, duration, bitrate;
} finfo_t;

enum MSG_T { ERROR, PLAIN, SUCCESS, DEBUG };
enum CONN_T { OUTGOING, INCOMING };
enum RESPONSE_T { ACK, AWAITING, ABORT };
enum CMDS { TERM, DEFCMD, SHOW, START, LOAD,
            SET, SET_CODEC, SET_SIZE,
          ASK, RESPOND, REACT,
          ASK_PEER, ASK_HOST,
          CONFIRM_PEER, CONFIRM_HOST,
          PING_PEER, PING_HOST,
          TRANSFER_PEER, TRANSFER_HOST };
class Command;
class NetworkCommand;
typedef std::map<std::string, std::string> configuration_t;
typedef std::map<CMDS, Command *> cmd_storage_t;
typedef std::map<CMDS, NetworkCommand *> net_cmd_storage_t;

struct MyAddr {
    std::string addr;
    int port;
    void print();
    std::string get();
    MyAddr(struct sockaddr_storage &addr);
};

struct NeighborInfo {
    struct sockaddr_storage address;
    time_t last_seen;
    int intervals;
    //quality
    bool confirmed;
    bool active;

    void printInfo();

    NeighborInfo(struct sockaddr_storage &addr) {
        address = addr;
        intervals = CHECK_INTERVALS;
    }
};


struct VideoState {
    finfo_t finfo;
    size_t secs_per_chunk, c_chunks, chunk_size;
    std::string dir_location, o_format, o_codec;
    bool working = false, splitting_ongoing = false, split_deq_used;
    int to_send;
    std::deque<std::string> chunks_to_process;
    std::mutex split_mtx;
    std::condition_variable split_cond;
    NetworkHandle *net_handler;
    VideoState(NetworkHandle *nh): secs_per_chunk(0), c_chunks(0), chunk_size(CHUNK_SIZE),
    o_format("mkv"), o_codec("h264"), split_deq_used(false) {
        net_handler = nh;
    }

    int split();
    int join();
    void printVideoState();
    void changeChunkSize(size_t nsize);
    void loadFileInfo(finfo_t &finfo);
    void resetFileInfo();
    void abort();
    void pushChunk(std::string path);
    void transferChunk();
};

void splitTransferRoutine(VideoState *st);

class HistoryStorage {
    std::vector<std::string> history;
    std::string filename;
    unsigned int c_index;
public:
    HistoryStorage(const std::string &fn);
    void prev();
    void next();
    void save(std::string line);
    void write();
    std::string &getCurrent();
};

typedef std::pair<std::string, MSG_T> printable_pair_T;

class WindowPrinter {
    WINDOW *win;
    std::deque<printable_pair_T> q;
    bool bolded;
public:
    enum DIRECTION {UP, DOWN} direction;
    enum START {TOP, BOTTOM} start;
    WindowPrinter(DIRECTION dir, bool b, START st): bolded(b), direction(dir), start(st){
        win = stdscr;
    }
    void changeWin(WINDOW *nwin);
    void add(std::string msg, MSG_T type);
    void print();
};

struct State {
    bool enough_neighbors = false;

};

struct IO_Data {
    IO_Data(): info_handler(WindowPrinter::DOWN, false, WindowPrinter::TOP),
        status_handler(WindowPrinter::UP, true, WindowPrinter::BOTTOM) {}
    WindowPrinter info_handler, status_handler;
    int status_y = 0, perc_y = 0, question_y = 0;
    WINDOW *status_win, *info_win;
};

struct Mutexes_Data {
    std::mutex IO_mtx, report_mtx;
    std::condition_variable cond;
    bool using_IO = false;
};

struct Configuration {
    bool is_superpeer = false, IPv4_ONLY;
    std::map<std::string, int> intValues;
    std::string working_dir, superpeer_addr;
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
    static std::vector<std::string> getKnownCodecs() {
        return {"libx264", "msmpeg"};
    }
    static std::map<wchar_t, CMDS> getCmdMapping() {
        return {
            { KEY_F(6), SHOW },
            { KEY_F(7), START },
            { KEY_F(8), LOAD },
            { KEY_F(10), ASK },
            { KEY_F(9), SET } };
    }

private:
    static std::shared_ptr<Data> inst;
};


#endif

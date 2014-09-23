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
#define DEBUGGING 1
#define STATUS_LENGTH 5
#define CHUNK_SIZE 40048576
#define INFO_LINES 15
#define LINE_LENGTH 80
#define LISTENING_PORT 25000
#define SUPERPEER_PORT 26000
#define SUPERPEER_ADDR "127.0.0.1"
#define BUF_LENGTH 256
#define DEFAULT 1
#define RED 2
#define GREEN 3
#define GREY 4
#define BLUE 5
#define YELLOWALL 6
#define GREYALL 7
#define CYANALL 8
#define COLOR_GREY 100
#define BG 20
#define BG_COL COLOR_BLACK
#define ST_Y 10

#define show(x, y) printw("%15s%35s\n", x, y);

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
enum CMDS { DEFCMD, SHOW, START, LOAD,
            SET, SET_CODEC, SET_SIZE,
          ASK, RESPOND, REACT,
          CONFIRM_PEER, CONFIRM_HOST };
class Command;
typedef std::map<std::string, std::string> configuration_t;
typedef std::map<CMDS, Command *> cmd_storage_t;

struct NeighborInfo {
    struct sockaddr_storage address;
    time_t last_seen;
    //quality
    bool confirmed;
    bool active;

    NeighborInfo(struct sockaddr_storage &addr) {
        address = addr;
    }
};

class VideoState {
public:
    finfo_t finfo;
    configuration_t configuration;
    size_t secs_per_chunk, c_chunks, chunk_size;
    std::string dir_location, o_format, o_codec;
    VideoState(configuration_t &conf): secs_per_chunk(0), c_chunks(0), chunk_size(CHUNK_SIZE),
    o_format("mkv"), o_codec("h264") {
        configuration = conf;
    }
    int split();
    int join();
    void printVideoState();
    void changeChunkSize(size_t nsize);
    void loadFileInfo(finfo_t &finfo);
    void resetFileInfo();
};

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

typedef std::pair<std::string, MSG_T> status_pairT;

class StatusInfo {
    std::deque<status_pairT> q;
public:
    void add(status_pairT msg);
    void print();
};

struct Data {
    Data() : status_y(0), perc_y(0), question_y(0) {}
    std::mutex input_mtx;

    std::condition_variable cond;
    bool reading_input = false;
    StatusInfo status_handler;
    cmd_storage_t cmds;
    int status_y, perc_y, question_y;
    static std::shared_ptr<Data> getInstance() {
        if(!inst)
            inst = std::shared_ptr<Data>(new Data);
        return inst;
    }
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

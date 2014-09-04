#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdio.h>
#include <time.h>
#include <curses.h>
#include <vector>


#ifndef DEFINES_H
#define DEFINES_H
#define CHUNK_SIZE 40048576

#define BUF_LENGTH 256
#define DEFAULT 1
#define RED 2
#define GREEN 3
#define GREY 4
#define BLUE 5
#define YELLOWALL 6

#define show(x, y) printw("%15s%35s\n", x, y);

namespace common {
    std::string getTimestamp();
}

typedef struct FileInfo {
    std::string fpath, codec, extension, basename, format;
    double fsize, duration, bitrate;
} finfo_t;

class Command;
typedef std::map<std::string, std::string> configuration_t;
typedef std::map<std::string, Command *> cmd_storage_t;

class State {
public:
    finfo_t finfo;
    configuration_t configuration;
    size_t secs_per_chunk, c_chunks, chunk_size;
    std::string dir_location, o_format, o_codec;
    State(configuration_t &conf): secs_per_chunk(0), c_chunks(0), chunk_size(CHUNK_SIZE),
    o_format("mkv"), o_codec("h264") {
        configuration = conf;
    }
    void printState();
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
#endif

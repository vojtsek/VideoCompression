#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdio.h>
#include <time.h>
#include <curses.h>


#ifndef DEFINES_H
#define DEFINES_H
#define CHUNK_SIZE 20048576

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
    void printState() {
        char value[BUF_LENGTH];
        if (!finfo.fpath.empty())
            show("File:", finfo.fpath.c_str());
        if (!finfo.codec.empty())
            show("Codec:", finfo.codec.c_str());
        if (finfo.bitrate) {
            snprintf(value, BUF_LENGTH, "%f", finfo.bitrate);
            show("Bitrate:", value);
        }
        if (finfo.duration) {
            snprintf(value, BUF_LENGTH, "%f", finfo.duration);
            show("Duration:", value);
        }
        if (finfo.fsize) {
            snprintf(value, BUF_LENGTH, "%f", finfo.fsize);
            show("File size:", value);
            snprintf(value, BUF_LENGTH, "%d", c_chunks);
            show("Chunks count:", value);
        }
        if (chunk_size) {
            snprintf(value, BUF_LENGTH, "%d", chunk_size / 1024);
            show("Chunk size:", value);
        }
        show("Output format:", o_format.c_str());
    }

    void loadFileInfo(finfo_t &finfo) {
        this->finfo = finfo;
        char dir_name[BUF_LENGTH];
        sprintf(dir_name, "job_%s", common::getTimestamp().c_str());
        dir_location = configuration["WD_PREFIX"] + "/" + std::string(dir_name);
        secs_per_chunk = CHUNK_SIZE  / finfo.bitrate;
        c_chunks = (((size_t) finfo.fsize) / chunk_size) + 1;
    }

    void resetFileInfo() {
        finfo.fpath = "";
        finfo.codec = "";
        finfo.bitrate = 0.0;
        finfo.duration = 0.0;
        finfo.fsize = 0.0;
        secs_per_chunk = 0;
        c_chunks = 0;
    }
};

#endif

#include <string>
#include <iostream>
#include <iomanip>


#ifndef DEFINES_H
#define DEFINES_H
#define CHUNK_SIZE 20048576
#define show(x,y) std::cout << std::setw(15) << x << std::setw(35) << y << std::endl

class State {
public:
    std::string fpath, codec, dir_location, extension, basename;
//    in bytes
    double fsize, duration, bitrate;
    size_t secs_per_chunk, c_chunks, chunk_size;
    State(): fsize(0.0), duration(0.0),bitrate(0.0),
        secs_per_chunk(0), c_chunks(0), chunk_size(CHUNK_SIZE) {}
    void printState() {
        if (!fpath.empty())
            show("File:", fpath.c_str());
        if (bitrate)
            show("Bitrate:", bitrate);
        if (duration)
            show("Duration:", duration);
        if (fsize) {
            show("File size:", fsize);
            c_chunks = (((size_t) fsize) / chunk_size) + 1;
            show("Chunks count:", c_chunks);
        }
        if (chunk_size)
            show("Chunk size: ", chunk_size / 1024);
    }

    void resetFileInfo() {
        fpath = "";
        codec = "";
        bitrate = 0.0;
        duration = 0.0;
        fsize = 0.0;
        secs_per_chunk = 0;
        c_chunks = 0;
    }
};

#endif

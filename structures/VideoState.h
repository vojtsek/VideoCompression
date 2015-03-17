#ifndef VIDEOSTATE_H
#define VIDEOSTATE_H

#include "headers/defines.h"
#include "structures/singletons.h"
#include "structures/structures.h"



struct VideoState {
    struct FileInfo finfo;
    size_t secs_per_chunk, c_chunks, chunk_size;
    int msgIndex, processed_chunks;
    std::string dir_location, job_id, o_format, o_codec;
    NetworkHandler *net_handler;
    VideoState(NetworkHandler *nh): secs_per_chunk(0), c_chunks(0),
        chunk_size(DATA->config.getValue("CHUNK_SIZE")),
    dir_location(DATA->config.working_dir), o_format(".mkv"), o_codec("libx264") {
        net_handler = nh;
    }

    int split();
    int join();
    void printVideoState();
    void changeChunkSize(size_t nsize);
    void loadFileInfo(struct FileInfo &finfo);
    void resetFileInfo();
    void abort();
};

#endif // VIDEOSTATE_H

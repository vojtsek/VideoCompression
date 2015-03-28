#ifndef VIDEOSTATE_H
#define VIDEOSTATE_H

#include "headers/defines.h"
#include "structures/singletons.h"
#include "structures/structures.h"



struct VideoState {
    struct FileInfo finfo;
    size_t secs_per_chunk, c_chunks, chunk_size;
    int32_t msgIndex, processed_chunks;
    std::string dir_location, job_id, o_format, o_codec;
    std::ofstream ofs;
    NetworkHandler *net_handler;
    VideoState(NetworkHandler *nh): secs_per_chunk(0), c_chunks(0),
        chunk_size(DATA->config.getIntValue("CHUNK_SIZE")), msgIndex(-1),
    dir_location(DATA->config.working_dir), o_format(".mkv"), o_codec("libx264") {
        net_handler = nh;
    }

    int32_t split();
    int32_t join();
    void printVideoState();
    void changeChunkSize(size_t nsize);
    void loadFileInfo(struct FileInfo &finfo);
    void resetFileInfo();
    void abort();
    void reportTime(std::string msg, int32_t time);
    void endProcess(int32_t duration);
};

#endif // VIDEOSTATE_H

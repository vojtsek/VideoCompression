#ifndef VIDEOSTATE_H
#define VIDEOSTATE_H

#include "headers/defines.h"
#include "structures/singletons.h"
#include "structures/structures.h"


/*!
 * \brief The VideoState struct
 * maintains the information regarding the video operations
 */
struct VideoState {
    //! holds the information about the loaded file
    struct FileInfo finfo;
    /*!
    * \var size_t secs_per_chunk how many seconds per one chunk
    * \var c_chunks number of chunks
    * \var chunk_size size of each chunk in bytes - actual chunks differ in size
    * \var msgIndex index of state message in the info handler
    * \var processed_chunks number of chuns that were already encoded and returned
    * \var dir_location location of the working directory
    * \var job_id string id of the current job
    * \var o_format file extension of the output - determines the container
    * \var o_codec codec used for encoding
    * \var ofs filestream pointing to the log file
    * \var net_handler reference to the NetworkHandler instance
    */
    int32_t secs_per_chunk, c_chunks, chunk_size,
    msgIndex, processed_chunks;
    std::string dir_location, job_id, o_format, o_codec;
    std::ofstream ofs;
    NetworkHandler *net_handler;
    VideoState(NetworkHandler *nh): secs_per_chunk(0), c_chunks(0),
        chunk_size(DATA->config.getIntValue("CHUNK_SIZE")), msgIndex(-1),
        processed_chunks(0), dir_location(DATA->config.working_dir),
      o_format(".mkv"), o_codec("libx264") {
        net_handler = nh;
    }

    /*!
     * \brief split handles splitting of the loaded file
     * \return 0 on success
     * computes the intervals for each chunk,
     * spawns ffmpeg instance for each chunk,
     * chunks are stored in $WD/$job_id
     * also queues the chunks for distribution
     */
    int32_t split();

    /*!
     * \brief join handles joining of the received chunks
     * \return 0 on success
     * generates the text file containing the chunk list,
     * then spawns the ffmpeg to join them
     * eventually calls endProcess()
     * the chunks are supposed to be located in
     * $WD/received/$job_id
     */
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

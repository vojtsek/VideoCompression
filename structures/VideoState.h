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
    //! flag that determines abortion of the process
    bool aborted;
    //! how many seconds per one chunk
    int64_t secs_per_chunk;
    //! number of chunks
    int64_t chunk_count;
    //! size of each chunk in bytes - actual chunks differ in size
    int64_t chunk_size;
    //! number of chunks that were already encoded and returned
    int64_t  processed_chunks;
    //! location of the working directory
    std::string dir_location;
    //! string id of the current job
    std::string job_id;
    //! file extension of the output - determines the container
    std::string o_format;
    //! codec used for encoding
    std::string o_codec;
    //! quality of the encoding
    std::string quality;
    //! filestream pointing to the log file
    std::ofstream ofs;
    //! to measure whole times
    std::chrono::system_clock::time_point start_time;
    //! reference to the NetworkHandler instance
    NetworkHandler *net_handler;

    VideoState(NetworkHandler *nh): aborted(false), secs_per_chunk(0), chunk_count(0),
        chunk_size(DATA->config.getIntValue("CHUNK_SIZE")),
        processed_chunks(0), dir_location(DATA->config.getStringValue("WD")),
      o_format(".mkv"), o_codec("libx264"), quality("ultrafast") {
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
    int64_t split();

    /*!
     * \brief join handles joining of the received chunks
     * \return 0 on success
     * generates the text file containing the chunk list,
     * then spawns the ffmpeg to join them
     * eventually calls endProcess()
     * the chunks are supposed to be located in
     * $WD/received/$job_id
     */
    int64_t join();

    /*!
     * \brief reset resets the state so new process can begin
     */
    void reset();

    /*!
     * \brief printVideoState
     * Prints information about loaded video file,
     * using WindowPrinter instance associated with
     * info window.
     */
    void printVideoState();

    /*!
     * \brief changeChunkSize sets the chunk size
     * \param nsize new size to set, in bytes
     * Computes the count of chunks and seconds per one chunk
     */
    void changeChunkSize(size_t nsize);

    /*!
     * \brief loadFileInfo sets current file info structure
     * \param finfo new file info structure to load
     */
    void loadFileInfo(struct FileInfo &finfo);

    /*!
     * \brief resetFileInfo resets all fields regarding video state
     */
    void resetFileInfo();

    /*!
     * \brief abort aborts the execution
     */
    void abort();

    /*!
     * \brief reportTime reports how long did the given operation took
     * \param msg message to show
     * \param time duration of the operation
     */
    void reportTime(std::string msg, int64_t time);
    /*!
     * \brief endProcess finishes the encoding process
     * \param duration how long did the process take
     * Call after the file is splitted,
     * reports information and
     * clears the structures.
     */
    void endProcess(int64_t duration);
};

#endif // VIDEOSTATE_H

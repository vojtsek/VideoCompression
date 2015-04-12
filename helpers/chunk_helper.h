#ifndef CHUNK_HELPER_H
#define CHUNK_HELPER_H

class NetworkHandler;
struct TransferInfo;

/*!
 * functions to handle chunks processing
 */
namespace chunkhelper {
    /**
     * @brief chunkhelper::chunkSendRoutine
     * @param net_handler
     *
     * This procedure is supposed to run in separate thread.
     * It uses the @struct SychronizedQueue, during each iteration it pops
     * next chunk to be sent. Then it checks, whether the chunks is addressed,
     * or not. In the latter case it tries to pick suitable neighbor for it,
     * queue it for resent otherwise.
     *
     * Adressed chunks are those, which are encoded already.
     * Then the connection is spawned and transfer begins.
     */
    void chunkSendRoutine(NetworkHandler *net_handler);

    /*!
     * \brief chunkhelper::chunkProcessRoutine
     * pops from the queue of chunks to encode and encode it.
     * then blocks waiting for next chunk
     */
    void chunkProcessRoutine();

    /*!
     * \brief chunkhelper::processReturnedChunk handles the return of the chunk
     * \param ti pointer to TransferInfo structure holding info about the returned chunk
     * \param state pointer to VideoState instance
     * removes the chunk from periodic listener i.e. stops checking
     * also ensures that it is not queued for send, unqueues it otherwise
     * deletes the old structure associated with the chunk and
     * saves the new to returned chunks
     *
     * updates quality of the neighbor who delivered this chunk
     */
    void pushChunk(TransferInfo *ti, std::mutex &mtx, std::condition_variable &cond,
               bool &ctrl_var, std::deque<TransferInfo *> &queue);

    /*!
     * \brief chunkhelper::pushChunkProcess enqueues the chunk for encoding
     * \param ti pointer to TransferInfo structure holding info about the chunk
     */
    void pushChunkProcess(TransferInfo *ti);

    /*!
     * \brief chunkhelper::pushChunkSend enqueues the chunk for sending
     * \param ti pointer to TransferInfo structure holding info about the chunk
     */
    void pushChunkSend(TransferInfo *ti);

    /*!
     * \brief chunkhelper::encodeChunk spawns and handles the encoding process
     * \param ti pointer to TransferInfo structure holding info about the chunk
     * \return zero on success
     *
     */
    void processReturnedChunk(TransferInfo *ti,
                  NetworkHandler *handler, VideoState *state);

    /*!
     * \brief chunkhelper::trashChunk destroys the TransferInfo structure safely
     * \param ti pointer to TransferInfo structure holding info about the chunk
     * \param delete whether delete the structure
     * Removes the TransferInfo pointer from storages
     * (except for returned_chunks) and queues and optionally
     * deletes the pointer
     */
    void trashChunk(TransferInfo *ti, bool del);

    /*!
     * \brief createChunk creates specified chunk, from currently loaded video
     * \param state pointer to corresponding video file
     * \param ti structure associated with the chunk
     * \param time accurate length of the chunk
     * \return actual time of the process
     */
    int64_t createChunk(VideoState *state,
                     TransferInfo *ti,
                     double *time);
    /*!
     * \brief getChunkDuration gets duration of the specified video file
     * \param path path to the chunk
     * \return duration in seconds
     */
    double getChunkDuration(const std::string &path);

    /*!
     * \brief chunkhelper::encodeChunk handles encoding of one chunk
     * \param ti pointer to TransferInfo structure holding info about the chunk
     * \return zero on success
     * Prepares the directory and spawns external process which
     * encodes the file.
     * Updates the corresponding TransferInfo,
     * removes the input file and queues the chunk to send.
     */
    int64_t encodeChunk(TransferInfo *ti);
}
#endif // CHUNK_HELPER_H

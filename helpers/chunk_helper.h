#ifndef CHUNK_HELPER_H
#define CHUNK_HELPER_H

class NetworkHandler;
struct TransferInfo;

void chunkSendRoutine(NetworkHandler *net_handler);
void chunkProcessRoutine();
void pushChunk(TransferInfo *ti, std::mutex &mtx, std::condition_variable &cond,
               bool &ctrl_var, std::deque<TransferInfo *> &queue);
void pushChunkProcess(TransferInfo *ti);
void pushChunkSend(TransferInfo *ti);
void processReturnedChunk(TransferInfo *ti,
                          NetworkHandler *handler, VideoState *state);
int encodeChunk(TransferInfo *ti);

#endif // CHUNK_HELPER_H

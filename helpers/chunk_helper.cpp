#include "defines.h"
#include "include_list.h"
#include "network_helper.h"

#include <string>
#include <fstream>
#include <utility>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>

#include <unistd.h>
#include <arpa/inet.h>
#include <curses.h>

using namespace std;
using namespace utilities;

void chunkSendRoutine(NetworkHandler *net_handler) {
    TransferInfo *ti;
    NeighborInfo *ngh;
    while (true) {
        unique_lock<mutex> lck(DATA->m_data.send_mtx, defer_lock);
        lck.lock();
        while (DATA->m_data.send_deq_used || !DATA->chunks_to_send.size()) {
            DATA->m_data.send_cond.wait(lck);
        }
        DATA->m_data.send_deq_used = true;
        ti = DATA->chunks_to_send.front();
        DATA->chunks_to_send.pop_front();
        DATA->m_data.send_deq_used = false;
        lck.unlock();
        DATA->m_data.send_cond.notify_one();
        if (!ti->addressed) {
            if (net_handler->getFreeNeighbor(ngh) == 0) {
                reportError("No free neighbor!");
                pushChunkSend(ti);
                //delete ti;
                sleep(5);
                continue;
            }
            int sock = net_handler->checkNeighbor(ngh->address);
            ti->address = ngh->address;
            //TODO get my address
            getHostAddr(ti->src_address, sock);
            ((struct sockaddr_in*) &ti->src_address)->sin_port =
                    htons(DATA->config.getValue("LISTENING_PORT"));
            net_handler->spawnOutgoingConnection(ngh->address, sock,
            { PING_PEER, DISTRIBUTE_PEER }, true, (void *) ti);
        } else {
            int sock = net_handler->checkNeighbor(ti->src_address);
            net_handler->spawnOutgoingConnection(ti->src_address, sock,
            { PING_PEER, RETURN_PEER }, true, (void *) ti);
            DATA->chunks_received.erase(ti->getHash());
        }
    }
}

void chunkProcessRoutine() {
    TransferInfo *ti;
    while (1) {
        unique_lock<mutex> lck(DATA->m_data.chunk_mtx, defer_lock);
        lck.lock();
        while (DATA->m_data.process_deq_used ||
               !DATA->chunks_to_encode.size() ||
               DATA->state.working)
            DATA->m_data.chunk_cond.wait(lck);
        DATA->m_data.process_deq_used = true;
        ti = DATA->chunks_to_encode.front();
        encodeChunk(ti);
        DATA->chunks_to_encode.pop_front();
        DATA->m_data.process_deq_used = false;
        lck.unlock();
        DATA->m_data.chunk_cond.notify_one();
    }
}

void processReturnedChunk(TransferInfo *ti,
                          NetworkHandler *handler, VideoState *state) {
    utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
              "/" + ti->name + ti->original_extension);
    DATA->periodic_listeners.erase(ti->getHash());
    DATA->waiting_chunks.erase(ti->getHash());
    unique_lock<mutex> lck(DATA->m_data.send_mtx, defer_lock);
    lck.lock();
    while (DATA->m_data.send_deq_used) {
        DATA->m_data.send_cond.wait(lck);
    }
    DATA->m_data.send_deq_used = true;
    for (auto it = DATA->chunks_to_send.begin();
         it != DATA->chunks_to_send.end(); ++it) {
        if ((*it)->getHash() == ti->getHash()) {
            DATA->chunks_to_send.erase(it);
            DATA->state.to_send--;
            break;
        }
    }
    DATA->m_data.send_deq_used = false;
    lck.unlock();
    DATA->m_data.send_cond.notify_one();
    int comp_time = atoi(utilities::getTimestamp().c_str())
        - atoi(ti->timestamp.c_str());
    NeighborInfo *ngh = handler->getNeighborInfo(ti->address);
    ngh->overall_time += comp_time;
    ngh->processed_chunks++;
    ngh->quality = ngh->overall_time / ngh->processed_chunks;
    reportDebug(MyAddr(ti->address).get() +
              " new quality: " + utilities::m_itoa(ngh->quality), 3);
    MSG_T type = DEBUG;
    if (!--DATA->state.to_recv) {
        type = SUCCESS;
    }
    DATA->io_data.info_handler.updateAt(state->msgIndex,
                        utilities::formatString(
                        "processed chunks:",
                        utilities::m_itoa(++state->processed_chunks) +
                        "/" + utilities::m_itoa(state->c_chunks)), type);
}

void pushChunk(TransferInfo *ti, std::mutex &mtx, std::condition_variable &cond,
               bool &ctrl_var, std::deque<TransferInfo *> &queue) {
    unique_lock<mutex> lck(mtx, defer_lock);

    lck.lock();
    while (ctrl_var)
        cond.wait(lck);
    ctrl_var = true;
    queue.push_back(ti);
    ctrl_var = false;
    lck.unlock();
    cond.notify_one();
}

void pushChunkProcess(TransferInfo * ti) {
    pushChunk(ti, DATA->m_data.chunk_mtx, DATA->m_data.chunk_cond,
              DATA->m_data.process_deq_used, DATA->chunks_to_encode);
}

void pushChunkSend(TransferInfo * ti) {
    pushChunk(ti, DATA->m_data.send_mtx, DATA->m_data.send_cond,
              DATA->m_data.send_deq_used, DATA->chunks_to_send);
}

int Configuration::getValue(string key) {
    try {
        return intValues.at(key);
    } catch (std::out_of_range e) {
        return 0;
    }
}

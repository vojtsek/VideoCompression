#include "headers/defines.h"
#include "headers/include_list.h"
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
        ti = DATA->chunks_to_send.pop();
        if (!ti->addressed) {
            if ((ngh = DATA->neighbors.getFreeNeighbor()) == nullptr) {
                reportDebug("No free neighbor!", 2);
                pushChunkSend(ti);
                sleep(5);
                continue;
            }
            int sock = net_handler->checkNeighbor(ngh->address);
            ti->address = ngh->address;
            getHostAddr(ti->src_address, sock);
            ((struct sockaddr_in*) &ti->src_address)->sin_port =
                    htons(DATA->config.getValue("LISTENING_PORT"));
            net_handler->spawnOutgoingConnection(ngh->address, sock,
            { PING_PEER, DISTRIBUTE_PEER }, true, (void *) ti);
            DATA->neighbors.setNeighborFree(ngh->address, false);
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
        ti = DATA->chunks_to_encode.pop();
        encodeChunk(ti);
    }
}

void processReturnedChunk(TransferInfo *ti,
                          NetworkHandler *, VideoState *state) {
    utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
              "/" + ti->name + ti->original_extension);
    DATA->periodic_listeners.erase(ti->getHash());
    DATA->waiting_chunks.erase(ti->getHash());
    if (DATA->chunks_to_send.remove(ti)) {
        DATA->state.to_send--;
    }
    int comp_time = atoi(utilities::getTimestamp().c_str())
        - atoi(ti->timestamp.c_str());
    DATA->neighbors.applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (cmpStorages(entry.second->address, ti->address)) {
            entry.second->overall_time += comp_time;
            entry.second->processed_chunks++;
            entry.second->quality = entry.second->overall_time /
                    entry.second->processed_chunks;
        }});
    MSG_T type = DEBUG;
    if (!--DATA->state.to_recv) {
        type = SUCCESS;
    }
    //TODO: add to free negihbors?
    DATA->io_data.info_handler.updateAt(state->msgIndex,
                        utilities::formatString(
                        "processed chunks:",
                        utilities::m_itoa(++state->processed_chunks) +
                        "/" + utilities::m_itoa(state->c_chunks)), type);
}

void pushChunkProcess(TransferInfo *ti) {
    DATA->chunks_to_encode.push(ti);
}

void pushChunkSend(TransferInfo *ti) {
    DATA->chunks_to_send.push(ti);
}

int Configuration::getValue(string key) {
    try {
        return intValues.at(key);
    } catch (std::out_of_range e) {
        return 0;
    }
}

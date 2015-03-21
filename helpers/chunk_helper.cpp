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
            //ngh should not be here
            DATA->periodic_listeners.remove(ti);
            int sock = net_handler->checkNeighbor(ngh->address);
            ti->address = ngh->address;
            getHostAddr(ti->src_address, sock);
            ((struct sockaddr_in*) &ti->src_address)->sin_port =
                    htons(DATA->config.getValue("LISTENING_PORT"));
            net_handler->spawnOutgoingConnection(ngh->address, sock,
            { PING_PEER, DISTRIBUTE_PEER }, false, (void *) ti);
            DATA->chunks_to_send.signal();
            DATA->periodic_listeners.push(ti);
            ti->sent_times++;
            DATA->neighbors.setNeighborFree(ngh->address, false);
            reportDebug(ti->name + " was sent.", 3);
        } else {
            reportSuccess("Returning: " + ti->getHash());
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
    DATA->periodic_listeners.remove(ti);
    reportDebug("Chunk returned: " + ti->getHash(), 2);
    DATA->chunks_returned.push(ti);
    utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
              "/" + ti->name + ti->original_extension);
    DATA->chunks_to_send.remove(ti);
    int comp_time = atoi(utilities::getTimestamp().c_str())
        - atoi(ti->timestamp.c_str());
    ti->encoding_time = comp_time;
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

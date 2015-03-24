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

/**
 * @brief chunkSendRoutine
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

void chunkSendRoutine(NetworkHandler *net_handler) {
    TransferInfo *ti;
    struct sockaddr_storage free_address;
    while (true) {
        ti = DATA->chunks_to_send.pop();
        if (DATA->chunks_to_send.contains(ti)) {
            continue;
        }
        if (!ti->addressed) {
            if (DATA->neighbors.getFreeNeighbor(
                     free_address) == 0) {
                reportDebug("No free neighbor!", 2);
                pushChunkSend(ti);
                sleep(5);
                continue;
            }
            DATA->periodic_listeners.remove(ti);
            int32_t sock = net_handler->checkNeighbor(free_address);
            ti->address = free_address;
            // get host address which is being used for communication
            // on this address the chunk should be returned.
            networkHelper::getHostAddr(ti->src_address, sock);
            networkHelper::changeAddressPort(ti->src_address,
                              DATA->config.getValue("LISTENING_PORT"));
            net_handler->spawnOutgoingConnection(free_address, sock,
            { PING_PEER, DISTRIBUTE_PEER }, true, (void *) ti);
            // start checking the processing time
            // is done here, so in case of failure it is handled
            DATA->periodic_listeners.push(ti);
            ti->sent_times++;
            // update neighbor information
            DATA->neighbors.setNeighborFree(free_address, false);
            reportDebug(ti->name + " was sent.", 3);
        } else {
            // in case of returning chunk
            int32_t sock = net_handler->checkNeighbor(ti->src_address);
            net_handler->spawnOutgoingConnection(ti->src_address, sock,
            { PING_PEER, RETURN_PEER }, true, (void *) ti);
            DATA->chunks_received.remove(ti);
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
    ti->processing_time = utilities::computeDuration(utilities::getTimestamp(), ti->timestamp);
    DATA->neighbors.applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (networkHelper::cmpStorages(entry.second->address, ti->address)) {
            entry.second->overall_time += ti->encoding_time;
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

int32_t encodeChunk(TransferInfo *ti) {
    std::string out, err, res_dir;
    char cmd[BUF_LENGTH];
    res_dir = DATA->config.working_dir + "/processed/" + ti->job_id;
    /*
    if (prepareDir(res_dir, false) == -1) {
        reportDebug("Failed to create working dir.", 2);
        return -1;
    }
    */
    if (utilities::prepareDir(res_dir, false) == -1) {
        reportDebug("Failed to create job dir.", 2);
        return -1;
    }
    std::string file_out = res_dir + "/" + ti->name + ti->desired_extension;
    std::string file_in = DATA->config.working_dir + "/to_process/" +
            ti->job_id + "/" + ti->name + ti->original_extension;
    reportDebug("Encoding: " + file_in, 2);
    snprintf(cmd, BUF_LENGTH, "/usr/bin/ffmpeg");
    unlink(file_out.c_str());
    int32_t duration = Measured<>::exec_measure(utilities::runExternal, out, err, cmd, 10, cmd,
             "-i", file_in.c_str(),
             "-c:v", ti->output_codec.c_str(),
             "-preset", "ultrafast",
             "-qp", "0",
             file_out.c_str());
    if (err.find("Conversion failed") != std::string::npos) {
        reportDebug("Failed to encode chunk!", 2);
        std::ofstream os(ti->job_id + ".err");
        os << err << std::endl;
        os.flush();
        os.close();
        //should retry?
        delete ti;
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        return -1;
    }
        reportStatus("Signaled.");
    reportDebug("Chunk " + ti->name + " encoded.", 2);
    ti->encoding_time = duration;
    utilities::rmFile(file_in);
    std::atomic_fetch_add(&DATA->state.can_accept, 1);
    pushChunkSend(ti);
    return 0;
}


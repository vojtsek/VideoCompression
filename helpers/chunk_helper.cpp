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

void chunkhelper::chunkSendRoutine(NetworkHandler *net_handler) {
    TransferInfo *ti;
    struct sockaddr_storage free_address;
    // should run in separate thread
    while (true) {
        // pops one chunk to send
        ti = DATA->chunks_to_send.pop();
        // assure no duplicates
                DATA->chunks_to_send.remove(ti);
        // first time send -> to process
        if (!ti->addressed) {
            // obtain free neighbor
            if (DATA->neighbors.getFreeNeighbor(
                     free_address) == 0) {
                // no free neighbor so try to gather some
                reportDebug("No free neighbors!", 2);
                struct sockaddr_storage maddr =
                        DATA->config.my_IP.getAddress();
                                    struct sockaddr_storage neighbor_addr;
                                    if (DATA->neighbors.getRandomNeighbor(neighbor_addr) == 0) {
                                            reportDebug("No neighbors!", 3);
                                    } else {
                                            net_handler->gatherNeighbors(
                                                                    DATA->config.getIntValue("TTL"), maddr, neighbor_addr);
                                    }
                // resend the chunk and wait a while, then try again
                chunkhelper::pushChunkSend(ti);
                sleep(2);
                continue;
            }
            // checks the neighbor and addresses the chunk
            int64_t sock = net_handler->checkNeighbor(free_address);
            ti->address = free_address;
            // get host address which is being used for communication
            // on this address the chunk should be returned.
            ti->src_address= DATA->config.my_IP.getAddress();
            networkHelper::changeAddressPort(ti->src_address,
                              DATA->config.getIntValue("LISTENING_PORT"));

            // send the chunk to process
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
            int64_t sock = net_handler->checkNeighbor(ti->src_address);
            net_handler->spawnOutgoingConnection(ti->src_address, sock,
            { PING_PEER, RETURN_PEER }, true, (void *) ti);
            // TODO: DATA->chunks_received.remove(ti);
        }
    }
}

void chunkhelper::chunkProcessRoutine() {
    TransferInfo *ti;
    while (1) {
        ti = DATA->chunks_to_encode.pop();
        chunkhelper::encodeChunk(ti);
    }
}

void chunkhelper::processReturnedChunk(TransferInfo *ti,
                          NetworkHandler *, VideoState *state) {
    chunkhelper::trashChunk(ti, false);
    // ti will be deleted while postprocessing

    reportDebug("Chunk returned: " + ti->toString(), 2);
    DATA->chunks_returned.push(ti);
    OSHelper::rmFile(DATA->config.getStringValue("WD") + "/" + ti->job_id +
              "/" + ti->name + ti->original_extension);
    // update quality
    DATA->neighbors.applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (networkHelper::cmpStorages(entry.second->address, ti->address)) {
            entry.second->overall_time += ti->encoding_time;
            entry.second->processed_chunks++;
            entry.second->quality = entry.second->overall_time /
                    entry.second->processed_chunks;
        }});
    // update counters
    --DATA->state.to_recv;
    ++state->processed_chunks;
    utilities::printOverallState(state);
}

void chunkhelper::pushChunkProcess(TransferInfo *ti) {
    // decrease number of acceptible chunks
    std::atomic_fetch_sub(&DATA->state.can_accept, 1);
    DATA->chunks_to_encode.push(ti);
}

void chunkhelper::pushChunkSend(TransferInfo *ti) {
    if (OSHelper::getFileSize(ti->path) != -1) {
        DATA->chunks_to_send.push(ti);
    }
}

int64_t chunkhelper::encodeChunk(TransferInfo *ti) {
    std::string out, err, res_dir;
    char cmd[BUF_LENGTH];
    res_dir = DATA->config.getStringValue("WD") + "/processed/" + ti->job_id;

    // TODO: try-catch
    if (OSHelper::prepareDir(res_dir, false) == -1) {
        reportDebug("Failed to create job dir.", 2);
        return -1;
    }
    // construct the paths
    std::string file_out = res_dir + "/" + ti->name + ti->desired_extension;
    std::string file_in = DATA->config.getStringValue("WD") + "/to_process/" +
            ti->job_id + "/" + ti->name + ti->original_extension;
    reportDebug("Encoding: " + file_in, 2);
    snprintf(cmd, BUF_LENGTH, "%s",
             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());

    // ensures, the desired file doesn't exist yet
    if (OSHelper::rmFile(file_out) == -1) {
        reportDebug("OS error while encoding " + ti->name, 2);
        return -1;
    }

    // spawns the encoding process
    int64_t duration = Measured<>::exec_measure(OSHelper::runExternal, out, err, cmd, 11, cmd,
             "-i", file_in.c_str(),
             "-c:v", ti->output_codec.c_str(),
             "-preset", "ultrafast",
             "-nostdin",
             "-qp", "0",
             file_out.c_str());
    // case of failure
    if (err.find("Conversion failed") != std::string::npos) {
        reportDebug("Failed to encode chunk!", 2);
        std::ofstream os(ti->job_id + ".err");
        os << err << std::endl;
        os.flush();
        os.close();
        //TODO: should retry?
        // TODO: inform
        // remove from structures and delete
        chunkhelper::trashChunk(ti, true);
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        return -1;
    }
    reportDebug("Chunk " + ti->name + " encoded.", 2);
    ti->encoding_time = duration;
    // removal of input file - no longer needed
    OSHelper::rmFile(file_in);
    // can accept one more chunk no
    std::atomic_fetch_add(&DATA->state.can_accept, 1);
    // will be send
    chunkhelper::pushChunkSend(ti);
    return 0;
}

void chunkhelper::trashChunk(TransferInfo *ti, bool del) {
    if (ti == nullptr) {
        return;
    }
    DATA->periodic_listeners.remove(ti);
    DATA->chunks_received.remove(ti);
    DATA->chunks_to_encode.remove(ti);
    DATA->chunks_to_send.remove(ti);
    if (del) {
        delete ti;
    }
}

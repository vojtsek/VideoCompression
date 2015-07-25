#include "headers/defines.h"
#include "headers/include_list.h"
#include "network_helper.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

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
    struct sockaddr_storage free_address;
    // should run in separate thread
    while (true) {
        TransferInfo *ti;
        // pops one chunk to send
        ti = DATA->chunks_to_send.pop();
        // assure no duplicates
                DATA->chunks_to_send.remove(ti);
                // already encoded in fact
                if (DATA->chunks_returned.contains(ti->name)) {
                    continue;
                }
        // sending the file was not succesful
        // first time send -> to process
        if (!ti->addressed) {
            // process was aborted
            if (!DATA->state.working) {
                continue;
            }
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
                                        // try to earn some neighbors
                                            net_handler->gatherNeighbors(
                                                                                                                    DATA->config.getIntValue("TTL"),
                                                        maddr, neighbor_addr);
                                            net_handler->obtainNeighbors();
                                    }
                // resend the chunk and wait a while, then try again
                chunkhelper::pushChunkSend(ti);
                sleep(2);
                continue;
            }
            // checks the neighbor and addresses the chunk
            int64_t sock = net_handler->checkNeighbor(free_address);
            if (sock == -1) {
                reportDebug("Failed to connect to neighbor", 2);
                chunkhelper::pushChunkSend(ti);
                sleep(2);
                continue;
            }
            DATA->neighbors.assignChunk(free_address, true, ti);
            ti->address = free_address;
            // get host address which is being used for communication
            // on this address the chunk should be returned.
            ti->src_address= DATA->config.my_IP.getAddress();
            // send the chunk to process
            net_handler->spawnOutgoingConnection(free_address, sock,
            { CMDS::PING_PEER, CMDS::DISTRIBUTE_PEER }, true, (void *) ti);
            // prevents to send more chunks to one neighbor in parallel, it becomes free after the transfer
                            DATA->neighbors.setNeighborFree(free_address, false);
        } else {
            // in case of returning chunk
            int64_t sock = net_handler->checkNeighbor(ti->src_address);
            if (sock == -1) {
                OSHelper::rmFile(ti->path);
                chunkhelper::trashChunk(ti, true);
                continue;
            }
            net_handler->spawnOutgoingConnection(ti->src_address, sock,
            { CMDS::PING_PEER, CMDS::RETURN_PEER }, true, (void *) ti);
            // removed when sent
        }
    }
}

void chunkhelper::chunkProcessRoutine() {
    while (1) {
        TransferInfo *ti;
        ti = DATA->chunks_to_encode.pop();
        chunkhelper::encodeChunk(ti);
    }
}

void chunkhelper::processReturnedChunk(TransferInfo *ti,
                          NetworkHandler *, TaskHandler *state) {
    ti->time_per_kb = (double)
            (ti->sending_time + ti->receiving_time + ti->encoding_time) *
            1024 / (double) ti->chunk_size;
    chunkhelper::trashChunk(ti, false);
    // ti will be deleted while postprocessing

    reportDebug("Chunk returned: " + ti->toString(), 2);
    DATA->chunks_returned.push(ti);
    OSHelper::rmFile(SPLITTED_PATH + PATH_SEPARATOR +
                     ti->name + ti->original_extension);
    DATA->neighbors.setNeighborFree(ti->address, true);
    // update quality
    DATA->neighbors.applyToNeighbors([&](
                     std::pair<std::string, NeighborInfo *> entry) {
        if (networkHelper::cmpStorages(entry.second->address, ti->address)) {
            entry.second->overall_time += ti->time_per_kb;
            entry.second->processed_chunks++;
            entry.second->quality = (double) entry.second->overall_time /
                    (double) entry.second->processed_chunks;
        }});
    // update counter
    state->processed_chunks = 
            DATA->chunks_returned.getSize();
    utilities::printOverallState(state);
}

void chunkhelper::pushChunkProcess(TransferInfo *ti) {
    // decrease number of acceptible chunks
    DATA->chunks_to_encode.push(ti);
}

void chunkhelper::pushChunkSend(TransferInfo *ti) {
            DATA->chunks_to_send.push(ti);
}

int64_t chunkhelper::createChunk(TaskHandler *state,
        TransferInfo *ti,
        double *time) {
    double retval;
    int64_t tries = TRIES_FOR_CHUNK, split_duration;
    std::string out, err;

    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH],
         current[BUF_LENGTH], msg[BUF_LENGTH], cmd[BUF_LENGTH];

    snprintf(chunk_duration, BUF_LENGTH, "%" PRId64,
             state->secs_per_chunk);
    snprintf(current, BUF_LENGTH, "%f", ti->start);
    snprintf(output, BUF_LENGTH, "%s",
             ti->path.c_str());
    snprintf(cmd, BUF_LENGTH, "%s",
             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());

        OSHelper::rmFile(ti->path);
    if (OSHelper::prepareDir(SPLITTED_PATH, false) == -1) {
        reportError("Failed to prepare directory while creating " +
                    ti->name);
        return -1;
    }
    while (tries-- > 0) {
            reportDebug("Creating chunk " +
                                    ti->path, 2);
            // spawn the command
            std::vector<std::string> args = {cmd,
                                                            "-ss", current,
                                                            "-i", state->finfo.fpath.c_str(),
                                                            "-v", "quiet",
                                                            "-c", "copy",
                                                            "-t", chunk_duration,
                                                            "-nostdin",
                                                            output };
            split_duration = Measured<>::exec_measure(OSHelper::runExternal,
                                                      out, err, state->secs_per_chunk * 2, args);
            // failure
            if ((err.find("Conversion failed") != std::string::npos) ||
                             (split_duration < 0)) {
                            snprintf(msg, BUF_LENGTH, "%s %s %s %s %s %s %s %sn",
                                                            DATA->config.getStringValue("FFMPEG_LOCATION").c_str(),
                                            "-i", state->finfo.fpath.c_str(),
                                            "-ss", current,
                                            "-t", chunk_duration,
                                            output);
                            reportError(msg);
                            break;
                    }
            // gets actual chunk duration
            retval = chunkhelper::getChunkDuration(output);
            if (retval < 0) {
                reportError("Failed to create " +
                             ti->path);
            } else {
                    break;
            }
    }
        if (tries <= 0) {
            reportError("Failed to split the file.");
            state->abort();
            return -1;
        }
        ti->duration = retval;
        *time = retval;
        ti->chunk_size = OSHelper::getFileSize(ti->path);
    return split_duration;
}

int64_t chunkhelper::encodeChunk(TransferInfo *ti) {
    std::string out, err, res_dir;
    char cmd[BUF_LENGTH];
    res_dir = PROCESSED_PATH;
    int64_t duration, tries = TRIES_FOR_CHUNK;
    std::string file_in, file_out;
    while (tries-- > 0) {
            if (OSHelper::prepareDir(res_dir, false) == -1) {
                    reportDebug("Failed to create job dir.", 2);
                    return -1;
            }
            // construct the paths
            file_out = ti->path;
            file_in = TO_PROCESS_PATH + PATH_SEPARATOR +
                    ti->name + ti->original_extension;
            reportDebug("Encoding: " + file_in, 2);
            snprintf(cmd, BUF_LENGTH, "%s",
                             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());

            // ensures, the desired file doesn't exist yet
            if (OSHelper::rmFile(file_out) == -1) {
                    reportDebug("OS error while encoding " + ti->name, 2);
                    return -1;
            }

            std::string a;
            for (uint i = 0; i < ti->run_args.size(); ++i) {
                try {
                    a = ti->run_args.at(i);
                } catch (std::out_of_range) {
                    reportError("Failed to process arguments");
                    return -1;
                }

                if (a == "IN") {
                    ti->run_args.at(i) = TO_PROCESS_PATH + PATH_SEPARATOR +
                            ti->name + ti->original_extension;
                } else if (a == "OUT") {
                    ti->run_args.at(i) = ti->path;
                }
            }
            // spawns the encoding process
            duration = Measured<>::exec_measure(
                        OSHelper::runExternal, out, err,
                                               ti->time_left * 2,
                                               ti->run_args);
            /*
            duration = Measured<>::exec_measure(
                        OSHelper::runExternal, out, err,
                        ti->time_left * 2, cmd, 13, cmd,
                             "-i", file_in.c_str(),
                             "-c:v", ti->output_codec.c_str(),
                             "-preset",
                             ti->quality.c_str(),
                                    "-c:a", "copy",
                             "-nostdin",
                             "-qp", "0",
                             file_out.c_str());
            */
            // case of failure
            if ((err.find("Conversion failed") != std::string::npos) ||
                    (duration < 0)) {
                    reportDebug("Failed to encode chunk!", 2);
                    std::ofstream os(ti->job_id + ".err");
                    os << err << std::endl;
                    os.flush();
                    os.close();
                    // another try
                    continue;
            }
            // succcess
            break;
    }

    // failed to encode
    if (tries <= 0) {
        chunkhelper::trashChunk(ti, true);
        OSHelper::rmFile(file_in);
        OSHelper::rmFile(file_out);
        return -1;
    }
    reportDebug("Chunk " + ti->name + " encoded.", 2);
    ti->encoding_time = duration;
    // removal of input file - no longer needed
    OSHelper::rmFile(file_in);
    // can accept one more chunk no
    // will be send
    chunkhelper::pushChunkSend(ti);
    return 0;
}

double chunkhelper::getChunkDuration(const string &fp) {
    std::string out, err, path = fp;
    rapidjson::Document document;
    // is file ok?
    if (OSHelper::checkFile(path) == -1){
        reportError("Loading the file " + path + " failed");
        return -1;
    }
    // gain some info about the video
    std::vector<std::string> args = {
        DATA->config.getStringValue("FFPROBE_LOCATION"),
        fp, "-show_format"
    };
    if (OSHelper::runExternal(out, err, 10, args) < 0) {
        reportError("Error while getting video information.");
        return -1;
    }
    if (err.find("Invalid data") != std::string::npos) {
        reportError("Invalid video file");
        return -1;
    }

    // parse the JSON output and save
    std::string dur_str(utilities::extract(
                            out, "duration", 1).at(0));
    try {
      double duration;
            std::stringstream ssd(dur_str.substr(dur_str.find("=") + 1));
            ssd >> duration;
            return duration;
    } catch (std::exception) {
        return -1;
    }
}

void chunkhelper::trashChunk(TransferInfo *ti, bool del) {
    if (ti == nullptr) {
        return;
    }
    // not removing from returned chunks
    DATA->periodic_listeners.remove(ti);
    DATA->chunks_received.remove(ti);
    DATA->chunks_to_encode.remove(ti);
    DATA->chunks_to_send.remove(ti);
    if (ti->assigned) {
        DATA->neighbors.applyToNeighbors([&](
                                         std::pair<std::string, NeighborInfo *> entry) {
            DATA->neighbors.removeNeighborChunk(entry.second, ti);
        });
    }
    if (del) {
        delete ti;
    }
}

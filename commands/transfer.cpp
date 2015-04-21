#include "headers/include_list.h"
#include <chrono>

using namespace utilities;

bool CmdDistributePeer::execute(int64_t fd, sockaddr_storage &address, void *) {
    CMDS action = CMDS::DISTRIBUTE_HOST;
    TransferInfo *ti(new TransferInfo);
    // is able to do some work?
    RESPONSE_T resp = networkHelper::isFree() ? RESPONSE_T::ACK_FREE : RESPONSE_T::ACK_BUSY;

    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    try {
        if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        // node is busy
        if (resp == RESPONSE_T::ACK_BUSY) {
            return true;
        }

        reportDebug("Beginning transfer. " + MyAddr(address).get(), 1);

        if (ti->receive(fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
        throw 1;
        }

        // this chunk has been received already
        if (DATA->chunks_received.contains(ti->toString())) {
            resp = RESPONSE_T::ABORT;
            if (sendResponse(fd, resp) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                throw 1;
            }
            reportDebug("I have this chunk already.", 2);
            return true;
        }
        DATA->chunks_received.push(ti);

        resp = RESPONSE_T::AWAITING;
        if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        std::string dir(TO_PROCESS_PATH);
        if (OSHelper::prepareDir(dir, false) == -1) {
            reportError(ti->name + ": Error creating job dir.");
            throw 1;
        }

        // receives the file, measures duration

        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        if (receiveFile(fd, dir + PATH_SEPARATOR +
                        ti->name + ti->original_extension) == -1) {
                    reportError(ti->name + ": Failed to transfer file.");
                    throw 1;
        }
        ti->sending_time = (std::chrono::duration_cast<std::chrono::milliseconds>
                                    (std::chrono::system_clock::now() - start)).count();

        reportDebug("Pushing chunk " + ti->name + "(" +
            utilities::m_itoa(ti->chunk_size) + ") to process.", 2);
        ti->path = PROCESSED_PATH + PATH_SEPARATOR +
                ti->name + ti->desired_extension;
        // this helps to determine, that the chunk should be send to src_adress
        // when popped from the queue later
        ti->addressed = true;
        // remember chunk
        utilities::printOverallState(state);
        chunkhelper::pushChunkProcess(ti);
    } catch (int) {
        DATA->chunks_received.remove(ti);
        return false;
    }

    return true;
}

bool CmdDistributeHost::execute(int64_t fd, sockaddr_storage &address, void *data) {
    RESPONSE_T resp;
    TransferInfo *ti = (TransferInfo *) data;

    // too many failed send attempts -> invalid file
        if (ti->tries_sent > CHUNK_RESENDS) {
            // it's "local" chunk, so it's essential
            ti->path = SPLITTED_PATH +
                    PATH_SEPARATOR + ti->name + ti->original_extension;
            if (!OSHelper::isFileOk(ti->path)) {
                // file is invalid, so the process has to be aborted
                            state->abort();
            } else {
                // "disable" neighbor, if is ok, it's state will be updated later
                DATA->neighbors.setNeighborFree(
                            ti->address, false);
                ti->tries_sent = 0;
                DATA->neighbors.assignChunk(ti->address, false, ti);
                chunkhelper::pushChunkSend(ti);
            }
            return false;
        }
        ti->tries_sent++;
    try {
        if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        // the peer became busy
        if (resp == RESPONSE_T::ACK_BUSY) {
            reportDebug("Peer is busy. " + MyAddr(address).get(), 2);
            DATA->neighbors.setNeighborFree(address, false);
            DATA->neighbors.assignChunk(address, false, ti);
            chunkhelper::pushChunkSend(ti);
            return true;
        }

                NeighborInfo *ngh = DATA->neighbors.getNeighborInfo(address);
        if (ngh == nullptr) {
            reportDebug("Invalid neighbor", 2);
            throw 1;
        }
        if (ngh->quality > 0) {
            ti->time_left = TIME_CONSTANT * ngh->quality *
                    // divide by 1000 for milliseconds, 1024 for kilobytes
                ti->chunk_size / (1024.0 * 1000);
        }
        if (ti->send(fd) == -1) {
            reportError(ti->name + ": Failed to send info.");
            throw 1;
        }

        if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        // neighbor has this chunk in queue already
        if (resp == RESPONSE_T::ABORT) {
            reportDebug(ti->name +
                ": This chunk is already being processed by this neighbor.", 2);
            ti->addressed = false;
            // lower the chance to pick same neighbor again
            // he is propably slow
            DATA->neighbors.updateQuality(
                        ti->address, 5);
            // resets the timeout
            ti->time_left = DATA->config.getIntValue(
                        "COMPUTATION_TIMEOUT");
            // returns true, assuming the neighbor will do its job
            // otherwise the periodic listener will be invoked later
            return true;
        }

                DATA->neighbors.setNeighborFree(address, false);
        if (sendFile(fd, ti->path) == -1) {
            reportError(ti->name + ": Failed to send.");
            throw 1;
        }
                DATA->periodic_listeners.push(ti);
        ti->sent_times++;
                reportDebug(ti->name + " was sent. " +
                                        MyAddr(address).get() +
                                        "; timeout: " + utilities::m_itoa((int) ti->time_left), 2);

    } catch (int) {
        // resend chunk in case of failure
        chunkhelper::pushChunkSend(ti);
        return false;
    }

    reportDebug(ti->name + ": Chunk transferred. " + m_itoa(
                    DATA->chunks_to_send.getSize()) + " remaining.", 2);
    return true;
}

bool CmdReturnHost::execute(
        int64_t fd, sockaddr_storage &address, void *data) {
    // pointer to encoded file is passed
    TransferInfo *ti = (TransferInfo *) data;
    RESPONSE_T resp;

    if (ti->tries_sent > CHUNK_RESENDS) {
        OSHelper::rmFile(ti->path);
        chunkhelper::trashChunk(ti, true);
        return false;
    }

        if (ti->send(fd) == -1) {
        reportError(ti->name + ": Failed to send info.");
        chunkhelper::pushChunkSend(ti);
        return false;
    }
        if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            chunkhelper::pushChunkSend(ti);
            return false;
        }

        if (resp == RESPONSE_T::ABORT) {
            reportDebug("Neighbor does not want this chunk." +
                        ti->name, 2);
            // remove files associated with this chunk
            OSHelper::rmFile(ti->path);
            OSHelper::rmFile(TO_PROCESS_PATH + PATH_SEPARATOR +
                             ti->name + ti->original_extension);
            chunkhelper::trashChunk(ti, true);
            return true;
        }

    ti->tries_sent++;
    if (sendFile(fd, ti->path) == -1) {
        reportError(ti->name + ": Failed to send.");
        chunkhelper::pushChunkSend(ti);
        return false;
    }

    // remove the file, no longer needed
    OSHelper::rmFile(ti->path);
    chunkhelper::trashChunk(ti, true);
    utilities::printOverallState(state);
    return true;
}

bool CmdReturnPeer::execute(
        int64_t fd, sockaddr_storage &address, void *) {
    CMDS action = CMDS::RETURN_HOST;
    RESPONSE_T resp = RESPONSE_T::AWAITING;
    TransferInfo helper_ti, *ti = nullptr;
    try {
        if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        if (helper_ti.receive(fd)) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        // get pointer to corresponding TransferInfo structure
        ti = (TransferInfo *) DATA->periodic_listeners.get(helper_ti.toString());
        // propably was received before
        if (ti == nullptr) {
            resp = RESPONSE_T::ABORT;
        }

        if (sendResponse(fd, resp) == -1) {
            reportDebug("Error while communicating with peer." + MyAddr(address).get(), 3);
            throw 1;
        }

        if (resp == RESPONSE_T::ABORT) {
            return true;
        }

        std::string dir(RECEIVED_PATH);
        if (OSHelper::prepareDir(dir, false) == -1) {
            reportError(ti->name + ": Error creating received job dir.");
            throw 1;
        }
        // update times
        ti->encoding_time = helper_ti.encoding_time;
        ti->sending_time = helper_ti.sending_time;
        // neighbor is treated as free
                DATA->neighbors.setNeighborFree(address, true);
        if (DATA->chunks_returned.contains(ti->toString())) {
                // the chunk returned before,
                // propably was resent to other neighbor - no longer needed
            reportError(ti->name + ": Too late.");
            return true;
        }
                std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        if (receiveFile(fd,
                dir + PATH_SEPARATOR + ti->name + ti->desired_extension) == -1) {
                    reportError(ti->name + ": Failed to transfer file.");
                    throw 1;
        }
        ti->receiving_time = (std::chrono::duration_cast<std::chrono::milliseconds>
                                    (std::chrono::system_clock::now() - start)).count();

        ti->path = dir + PATH_SEPARATOR + ti->name + ti->desired_extension;

        // this is first time the chunk returned
                chunkhelper::processReturnedChunk(ti, handler, state);
        // remove chunk association
        DATA->neighbors.assignChunk(address, false, ti);
                // all chunks has returned
                if ((state->processed_chunks == state->chunk_count) &&
                    (state->chunk_count != 0)) {
                        state->join();
                }
    } catch (int) {
        return false;
    }
    return true;
}

bool CmdGatherNeighborsPeer::execute(
        int64_t fd, sockaddr_storage &address, void *) {
    CMDS action = CMDS::GATHER_HOST;
    MyAddr requester_maddr;
    struct sockaddr_storage requester_addr;

    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    // receive requester's address
    if (requester_maddr.receive(fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    // "unwrap" sockaddr_storage structure
    requester_addr = requester_maddr.getAddress();
    // free to work, so ping the requester
    if (networkHelper::isFree()) {
        int64_t sock;
        if ((sock = handler->checkNeighbor(requester_addr)) == -1) {
            reportDebug("Failed to contact: " + requester_maddr.get(), 3);
        } else {
            handler->spawnOutgoingConnection(requester_addr, sock,
            { CMDS::PING_PEER }, true, nullptr);
        }
    }

    // lower the TTL
    if (--requester_maddr.TTL <= 0) {
        return true;
    }

    // how many neighbors contact
    int64_t count = DATA->neighbors.getNeighborCount() >
            DATA->config.getIntValue("UPP_LIMIT") ?
                DATA->config.getIntValue("UPP_LIMIT") : DATA->neighbors.getNeighborCount();
    // spread the message
    for (const auto &addr :
         DATA->neighbors.getNeighborAdresses(count)) {
        handler->gatherNeighbors(requester_maddr.TTL,
                    requester_addr, addr);
    }
    return true;
}

bool CmdGatherNeighborsHost::execute(
        int64_t fd, sockaddr_storage &address, void *data) {
    MyAddr *requester_addr = (MyAddr *) data;

    // invoked by networkHelper::gatherNeighbors()
    // the passed data contains address of the requester
    if (requester_addr->send(fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    delete requester_addr;
    return true;
}

#include "headers/include_list.h"
#include <chrono>

using namespace utilities;

bool CmdDistributePeer::execute(int64_t fd, sockaddr_storage &address, void *) {
    CMDS action = DISTRIBUTE_HOST;
    TransferInfo *ti(new TransferInfo);
    // is able to do some work?
    RESPONSE_T resp = networkHelper::isFree() ? ACK_FREE : ACK_BUSY;

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
        if (resp == ACK_BUSY) {
            return true;
        }

        reportDebug("Beginning transfer. " + MyAddr(address).get(), 1);

        if (ti->receive(fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
        throw 1;
        }

        // this chunk has been received already
        if (DATA->chunks_received.contains(ti->toString())) {
            resp = ABORT;
            if (sendResponse(fd, resp) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                throw 1;
            }
            reportDebug("I have this chunk already.", 2);
            throw 1;
        }

        resp = AWAITING;
        if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        std::string dir(DATA->config.getStringValue("WD") + "/to_process/" + ti->job_id);
        if (OSHelper::prepareDir(dir, false) == -1) {
            reportError(ti->name + ": Error creating job dir.");
            throw 1;
        }

        // receives the file, measures duration

        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        if (receiveFile(fd, dir + "/" + ti->name + ti->original_extension) == -1) {
                    reportError(ti->name + ": Failed to transfer file.");
                    throw 1;
        }
        ti->sending_time = (std::chrono::duration_cast<std::chrono::milliseconds>
                                    (std::chrono::system_clock::now() - start)).count();

        reportDebug("Pushing chunk " + ti->name + "(" +
            utilities::m_itoa(ti->chunk_size) + ") to process.", 2);
        ti->path = std::string(DATA->config.getStringValue("WD") + "/processed/" + ti->job_id +
                   "/" + ti->name + ti->desired_extension);
        // this helps to determine, that the chunk should be send to src_adress
        // when popped from the queue later
        ti->addressed = true;
        // remember chunk
        DATA->chunks_received.push(ti);
        utilities::printOverallState(state);
        chunkhelper::pushChunkProcess(ti);
    } catch (int) {
        return false;
    }

    return true;
}

bool CmdDistributeHost::execute(int64_t fd, sockaddr_storage &address, void *data) {
    RESPONSE_T resp;
    TransferInfo *ti = (TransferInfo *) data;

    try {
        if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        // the peer became busy
        if (resp == ACK_BUSY) {
            reportDebug("Peer is busy. " + MyAddr(address).get(), 2);
            DATA->neighbors.setNeighborFree(address, false);
            chunkhelper::pushChunkSend(ti);
            return true;
        }

        if (ti->send(fd) == -1) {
            reportError(ti->name + ": Failed to send info.");
            throw 1;
        }

        if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        // problems occured
        if (resp == ABORT) {
            reportDebug(
                "This chunk is already being processed by this neighbor.", 2);
            ti->addressed = false;
            // lower the chance to pick same neighbor again
            // he is propably slow
            DATA->neighbors.updateQuality(
                        ti->address, 5);
            throw 1;
        }

        if (sendFile(fd, ti->path) == -1) {
            reportError(ti->name + ": Failed to send.");
            throw 1;
        }
    } catch (int) {
        // resend chunk in case of failure
        chunkhelper::pushChunkSend(ti);
        return false;
    }

    reportDebug("Chunk transferred. " + m_itoa(
                    DATA->chunks_to_send.getSize()) + " remaining.", 2);
    return true;
}

bool CmdReturnHost::execute(
        int64_t fd, sockaddr_storage &, void *data) {
    // pointer to encoded file is passed
    TransferInfo *ti = (TransferInfo *) data;
    if (ti->send(fd) == -1) {
        reportError(ti->name + ": Failed to send info.");
        chunkhelper::pushChunkSend(ti);
        return false;
    }

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
    CMDS action = RETURN_HOST;
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
            throw 1;
        }

        std::string dir(DATA->config.getStringValue("WD") + "/received/" + ti->job_id);
        if (OSHelper::prepareDir(dir, false) == -1) {
            reportError(ti->name + ": Error creating received job dir.");
            throw 1;
        }
        ti->encoding_time = helper_ti.encoding_time;
        ti->sending_time = helper_ti.sending_time;

                std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        if (receiveFile(fd,
                dir + "/" + ti->name + ti->desired_extension) == -1) {
                    reportError(ti->name + ": Failed to transfer file.");
                    throw 1;
        }
        ti->receiving_time = (std::chrono::duration_cast<std::chrono::milliseconds>
                                    (std::chrono::system_clock::now() - start)).count();

        ti->path = dir + "/" + ti->name + ti->desired_extension;

        //handler->confirmNeighbor(ti->address);
        // this is first time the chunk returned
        if (!DATA->chunks_returned.contains(ti->toString())) {
            chunkhelper::processReturnedChunk(ti, handler, state);
            // all chunks has returned
            if (!DATA->state.to_recv) {
                state->join();
            }
            // the chunk returned before,
            // propably was resent to other neighbor - no longer needed
        } else {
            reportError(ti->name + ": Too late.");
        }
    } catch (int) {
        return false;
    }
    return true;
}

bool CmdGatherNeighborsPeer::execute(
        int64_t fd, sockaddr_storage &address, void *) {
    CMDS action = GATHER_HOST;
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
            { PING_PEER }, true, nullptr);
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

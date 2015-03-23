#include "headers/include_list.h"

using namespace utilities;

bool CmdDistributePeer::execute(int32_t fd, sockaddr_storage &address, void *) {
    CMDS action = DISTRIBUTE_HOST;
    TransferInfo *ti(new TransferInfo);
    RESPONSE_T resp = ACK_FREE;
    int32_t can_accept = std::atomic_fetch_sub(&DATA->state.can_accept, 1);
    if ((can_accept <= 0) || (DATA->state.working)) {
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        resp = ACK_BUSY;
    }
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    try {
    if (sendResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
    }

    if (resp == ACK_BUSY)
        return true;
    reportDebug("Beginning transfer. " + MyAddr(address).get(), 1);

    if (ti->receive(fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        throw 1;
    }

    if (DATA->chunks_received.contains(ti->getHash())) {
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

    std::string dir(DATA->config.working_dir + "/to_process/" + ti->job_id);
    if (prepareDir(dir, false) == -1) {
        reportError(ti->name + ": Error creating job dir.");
        throw 1;
    }
    if (receiveFile(fd, dir + "/" + ti->name + ti->original_extension) == -1) {
        reportError(ti->name + ": Failed to transfer file.");
        throw 1;
    }

    reportDebug("Pushing chunk " + ti->name + "(" +
                utilities::m_itoa(ti->chunk_size) + ") to process.", 2);
    ti->path = std::string(DATA->config.working_dir + "/processed/" + ti->job_id +
                           "/" + ti->name + ti->desired_extension);
    ti->addressed = true;
    //todo remove when done
    DATA->chunks_received.push(ti);
    pushChunkProcess(ti);
    } catch (int) {
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        return false;
    }

    return true;
}

bool CmdDistributeHost::execute(int32_t fd, sockaddr_storage &address, void *data) {
    RESPONSE_T resp;
    TransferInfo *ti = (TransferInfo *) data;

    try {
        if (receiveResponse(fd, resp) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            throw 1;
        }

        if (resp == ACK_BUSY) {
            reportDebug("Peer is busy. " + MyAddr(address).get(), 2);
            DATA->neighbors.setNeighborFree(address, false);
            pushChunkSend(ti);
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
        if (resp == ABORT) {
            reportDebug(
                "This chunk is already being processed by this neighbor.", 2);
            ti->addressed = false;
            DATA->neighbors.updateQuality(
                        ti->address, 5);
            throw 1;
        }

        ti->timestamp = utilities::getTimestamp();
        if (sendFile(fd, DATA->config.working_dir + "/" + ti->job_id +
             "/" + ti->name + ti->original_extension) == -1) {
            reportError(ti->name + ": Failed to send.");
            throw 1;
        }
        ti->sending_time = utilities::computeDuration(
                    utilities::getTimestamp(), ti->timestamp);
    } catch (int) {
        pushChunkSend(ti);
        return false;
    }


   // utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
    //                  "/" + ti->name + ti->original_extension);
    reportDebug("Chunk transferred. " + m_itoa(
                    DATA->chunks_to_send.getSize()) + " remaining.", 2);
    return true;
}
//TODO:generic function to send chunk

bool CmdReturnHost::execute(int32_t fd, sockaddr_storage &, void *data) {
    TransferInfo *ti = (TransferInfo *) data;
    if (ti->send(fd) == -1) {
        reportError(ti->name + ": Failed to send info.");
        pushChunkSend(ti);
        return false;
    }

    if (sendFile(fd, ti->path) == -1) {
        reportError(ti->name + ": Failed to send.");
        pushChunkSend(ti);
        return false;
    }

    DATA->chunks_to_send.signal();
    utilities::rmFile(ti->path);
    //delete ti;
    return true;
}

bool CmdReturnPeer::execute(int32_t fd, sockaddr_storage &address, void *) {
    CMDS action = RETURN_HOST;
    //TODO: delete in case of failure
    TransferInfo *ti(new TransferInfo);
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (ti->receive(fd)) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    std::string dir(DATA->config.working_dir + "/received/" + ti->job_id);
    /*if (prepareDir(dir false) == -1) {
        reportError(ti->name + ": Error creating received dir.");
        return false;
    }*/
    if (prepareDir(dir, false) == -1) {
        reportError(ti->name + ": Error creating received job dir.");
        return false;
    }
    ti->timestamp = utilities::getTimestamp();
    if (receiveFile(fd, dir + "/" + ti->name + ti->desired_extension) == -1) {
        reportError(ti->name + ": Failed to transfer file.");
        return false;
    }
    ti->receiving_time = utilities::computeDuration(utilities::getTimestamp(), ti->timestamp);
    // do I need two containers?
    if (!DATA->chunks_returned.contains(ti->getHash())) {
        processReturnedChunk(ti, handler, state);
        if (!DATA->state.to_recv) {
            state->join();
        }
    } else {
        reportError(ti->name + ": Too late.");
        //utilities::rmFile(dir + "/" + ti->name + ti->desired_extension);
    }

    return true;
}

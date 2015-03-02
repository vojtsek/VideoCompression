#include "include_list.h"
#include "network_helper.h"

using namespace utilities;

bool CmdTransferPeer::execute(int fd, sockaddr_storage &address, void *) {
    CMDS action = TRANSFER_HOST;
    std::string fn, job_id, chunk_name;
    RESPONSE_T resp = state->working ? ACK_BUSY : ACK_FREE;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp == ACK_FREE) {
        std::string answer;
        while (true) {
            answer = loadInput("", "Accept transfer?(y/n)", false);
            if ((answer == "y") || (answer == "n"))
                break;
        }
        if (answer == "n")
            resp = ACK_BUSY;
    }

    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (resp == ACK_BUSY)
        return true;
    reportStatus("Allowing transfer. " + MyAddr(address).get());

    if (!(fn = receiveString(fd)).size()) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    job_id = fn.substr(0, fn.find("/"));
    chunk_name = getBasename(fn);
    std::string dir(DATA->config.working_dir + "/to_process/" + job_id);
    if (prepareDir(dir) == -1) {
        reportError(fn + ": Error receiving chunk.");
        return false;
    }

    if (receiveFile(fd, dir + "/" + chunk_name) == -1) {
        reportError(fn + ": Failed to transfer file.");
        return false;
    }

    reportDebug("Pushing chunk " + fn + " to process.", 2);
    //pushChunkProcess(new TransferInfo(address, job_id, "libx264", fn));
    return true;
}

bool CmdTransferHost::execute(int fd, sockaddr_storage &address, void *data) {
    RESPONSE_T resp;
    std::string fn = (*(std::string *) data);
    delete (std::string *) data;
    std::string job_id = fn.substr(0, fn.find("/"));
    std::string chunk_name = getBasename(fn);

    if (recvSth(resp, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        state->pushChunk(fn);
        return false;
    }

    if (resp == ACK_BUSY) {
        reportError("Peer is busy. " + MyAddr(address).get());
        handler->setNeighborFree(address, false);
        state->pushChunk(fn);
        return true;
    }

    if (sendString(fd, fn) == -1) {
        reportError(fn + ": Failed to send filename.");
        state->pushChunk(fn);
        return false;
    }

    if (sendFile(fd, DATA->config.working_dir + "/" + fn) == -1) {
        reportError(fn + ": Failed to send.");
        state->pushChunk(fn);
        return false;
    }

    state->to_send--;
    std::unique_lock<std::mutex> lck(state->split_mtx, std::defer_lock);
    lck.lock();
    lck.unlock();
    state->split_cond.notify_one();
    TransferInfo *ti = new TransferInfo(address, fn, "", fn);
    DATA->periodic_listeners.insert(std::make_pair(fn, ti));
    DATA->waiting_chunks.insert(std::make_pair(fn, ti));
    reportSuccess("Chunk transferred. " + m_itoa(state->to_send) + " remaining.");
    handler->setNeighborFree(address, false);
    //TODO: delete
    DATA->waiting_chunks.erase(fn);
    return true;
}

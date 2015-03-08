#include "include_list.h"
#include "network_helper.h"

using namespace utilities;

bool CmdDistributePeer::execute(int fd, sockaddr_storage &address, void *) {
    CMDS action = DISTRIBUTE_HOST;
    TransferInfo *ti = new TransferInfo;
    RESPONSE_T resp = ACK_FREE;
    int can_accept = std::atomic_fetch_sub(&DATA->state.can_accept, 1);
    if ((can_accept <= 0) || (DATA->state.working)) {
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        resp = ACK_BUSY;
    }
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
   /*
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
    */
    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (resp == ACK_BUSY)
        return true;
    reportStatus("Allowing transfer. " + MyAddr(address).get());

    if (ti->receive(fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    std::string dir(DATA->config.working_dir + "/to_process");
    if (prepareDir(dir, false) == -1) {
        reportError(ti->name + ": Error creating received dir.");
        return false;
    }
    dir += "/" + ti->job_id;
    if (prepareDir(dir, false) == -1) {
        reportError(ti->name + ": Error creating job dir.");
        return false;
    }
    if (receiveFile(fd, dir + "/" + ti->name + ti->original_extension) == -1) {
        reportError(ti->name + ": Failed to transfer file.");
        return false;
    }

    reportDebug("Pushing chunk " + ti->name + "(" +
                utilities::m_itoa(ti->chunk_size) + ") to process.", 2);
    ti->path = std::string(DATA->config.working_dir + "/processed/" + ti->job_id +
                           "/" + ti->name + ti->desired_extension);
    ti->addressed = true;
    pushChunkProcess(ti);
    return true;
}

bool CmdDistributeHost::execute(int fd, sockaddr_storage &address, void *data) {
    RESPONSE_T resp;
    TransferInfo *ti = (TransferInfo *) data;

    if (recvSth(resp, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        pushChunkSend(ti);
        return false;
    }

    if (resp == ACK_BUSY) {
        reportError("Peer is busy. " + MyAddr(address).get());
        handler->setNeighborFree(address, false);
        pushChunkSend(ti);
        return true;
    }

    if (ti->send(fd) == -1) {
        reportError(ti->name + ": Failed to send info.");
        pushChunkSend(ti);
        return false;
    }

    if (sendFile(fd, DATA->config.working_dir + "/" + ti->job_id +
                 "/" + ti->name + ti->original_extension) == -1) {
        reportError(ti->name + ": Failed to send.");
        pushChunkSend(ti);
        return false;
    }

    std::unique_lock<std::mutex> lck(DATA->m_data.send_mtx, std::defer_lock);
    lck.lock();
    lck.unlock();
    DATA->m_data.send_cond.notify_one();
    DATA->periodic_listeners.insert(std::make_pair(ti->name, ti));
    DATA->waiting_chunks.insert(std::make_pair(ti->name, ti));
   // utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
    //                  "/" + ti->name + ti->original_extension);
    reportDebug("Chunk transferred. " + m_itoa(--DATA->state.to_send) + " remaining.", 2);
    return true;
}
//TODO:generic function to send chunk

bool CmdReturnHost::execute(int fd, sockaddr_storage &address, void *data) {
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

    std::unique_lock<std::mutex> lck(DATA->m_data.send_mtx, std::defer_lock);
    lck.lock();
    lck.unlock();
    DATA->m_data.send_cond.notify_one();
    utilities::rmFile(ti->path);
    delete ti;
    return true;
}

bool CmdReturnPeer::execute(int fd, sockaddr_storage &address, void *) {
    CMDS action = RETURN_HOST;
    TransferInfo *ti = new TransferInfo;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (ti->receive(fd)) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    std::string dir(DATA->config.working_dir + "/received/");
    if (prepareDir(dir, false) == -1) {
        reportError(ti->name + ": Error creating received dir.");
        return false;
    }
    dir += ti->job_id;
    if (prepareDir(dir, false) == -1) {
        reportError(ti->name + ": Error creating received job dir.");
        return false;
    }

    if (receiveFile(fd, dir + "/" + ti->name + ti->desired_extension) == -1) {
        reportError(ti->name + ": Failed to transfer file.");
        return false;
    }
    // do I need two containers?
    if (DATA->waiting_chunks.find(ti->name) !=
            DATA->waiting_chunks.end()) {
        utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
                              "/" + ti->name + ti->original_extension);
        DATA->periodic_listeners.erase(ti->name);
        DATA->waiting_chunks.erase(ti->name);
        if (!--DATA->state.to_recv) {
            reportSuccess("All chunks are back!");
            state->join();
        }
    } else {
        reportError(ti->name + ": Too late.");
        //utilities::rmFile(dir + "/" + ti->name + ti->desired_extension);
    }

    return true;
}

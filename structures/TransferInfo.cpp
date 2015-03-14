#include "defines.h"
#include "include_list.h"

void TransferInfo::print() {
    reportStatus("Path: " + path);
    reportStatus("Chunk name: " + name);
    MyAddr srca(src_address), a(address);
    reportStatus("SRC ADDR: " + srca.get());
    reportStatus("ADDR: " + a.get());
}

void TransferInfo::invoke(NetworkHandler &handler) {
    if (--time_left <= 0) {
        if ((--tries_left > 0) && (handler.checkNeighbor(address) != -1)) {
            time_left = DATA->config.getValue(
                        "COMPUTATION_TIMEOUT");
            return;
        } else {

        }
        try {
            DATA->waiting_chunks.at(getHash());
            reportError(name + ": Still in queue, resending.");
            DATA->state.to_send++;
            pushChunkSend(this);
            time_left = DATA->config.getValue(
                        "COMPUTATION_TIMEOUT");
            tries_left = DATA->config.getValue(
                        "TRIES_BEFORE_RESEND");
        } catch (...) {}
    }
}

int TransferInfo::send(int fd) {
    if (sendSth(chunk_size, fd) == -1) {
        reportDebug("Failed to send size.", 1);
        return -1;
    }

    if (sendSth(address, fd) == -1) {
        reportDebug("Failed to send source address.", 1);
        return -1;
    }

    if (sendSth(src_address, fd) == -1) {
        reportDebug("Failed to send source address.", 1);
        return -1;
    }

    if (sendString(fd, job_id) == -1) {
        reportDebug("Failed to send job id.", 1);
        return -1;
    }

    if (sendString(fd, name) == -1) {
        reportDebug("Failed to send name.", 1);
        return -1;
    }

    if (sendString(fd, original_extension) == -1) {
        reportDebug("Failed to send original extension.", 1);
        return -1;
    }

    if (sendString(fd, desired_extension) == -1) {
        reportDebug("Failed to send desired extension.", 1);
        return -1;
    }

    if (sendString(fd, output_codec) == -1) {
        reportDebug("Failed to send codec.", 1);
        return -1;
    }

    if (sendString(fd, timestamp) == -1) {
        reportDebug("Failed to send codec.", 1);
        return -1;
    }
    return 0;
}

int TransferInfo::receive(int fd) {
    if (recvSth(chunk_size, fd) == -1) {
        reportDebug("Failed to receive size.", 1);
        return -1;
    }
    struct sockaddr_storage srca;

    if (recvSth(srca, fd) == -1) {
        reportDebug("Failed to receive source address.", 1);
        return -1;
    }
    address = srca;

    if (recvSth(srca, fd) == -1) {
        reportDebug("Failed to receive source address.", 1);
        return -1;
    }

    src_address = srca;

    if ((job_id = receiveString(fd)).empty()) {
        reportDebug("Failed to receive job id.", 1);
        return -1;
    }

    if ((name = receiveString(fd)).empty()) {
        reportDebug("Failed to receive name.", 1);
        return -1;
    }

    if ((original_extension = receiveString(fd)).empty()) {
        reportDebug("Failed to receive original extension.", 1);
        return -1;
    }

    if ((desired_extension = receiveString(fd)).empty()) {
        reportDebug("Failed to receive desired extension.", 1);
        return -1;
    }

    if ((output_codec = receiveString(fd)).empty()) {
        reportDebug("Failed to receive codec.", 1);
        return -1;
    }

    if ((timestamp = receiveString(fd)).empty()) {
        reportDebug("Failed to receive codec.", 1);
        return -1;
    }
    return 0;
}

std::string TransferInfo::getHash() {
    return (name + job_id);
}

#include "headers/defines.h"
#include "headers/include_list.h"
#include <sstream>

void TransferInfo::print() {
    reportStatus("Path: " + path);
    reportStatus("Chunk name: " + name);
    MyAddr srca(src_address), a(address);
    reportStatus("SRC ADDR: " + srca.get());
    reportStatus("ADDR: " + a.get());
}

void TransferInfo::invoke(NetworkHandler &handler) {
    time_left -= TICK_DURATION;
    // timeout is up
    if (time_left <= 0) {
        // the are still some tries left -> try to contact the neighbor
        if ((--tries_left > 0) && (handler.checkNeighbor(address) != -1)) {
            time_left = DATA->config.getIntValue(
                        "COMPUTATION_TIMEOUT");
            return;
        }
        try {
            // it has already returned
            if (DATA->chunks_returned.contains(toString())) {
                return;
            }
            reportDebug(name + ": Still in queue, resending.", 2);
            // push for resend
            chunkhelper::pushChunkSend(this);
            time_left = DATA->config.getIntValue(
                        "COMPUTATION_TIMEOUT");
            tries_left = DATA->config.getIntValue(
                        "TRIES_BEFORE_RESEND");
        } catch (...) {}
    }
}

int64_t TransferInfo::send(int64_t fd) {
    if (sendInt64(fd, chunk_size) == -1) {
        reportDebug("Failed to send size.", 1);
        return -1;
    }

    if (sendInt64(fd, sent_times) == -1) {
        reportDebug("Failed to send times sent.", 1);
        return -1;
    }

    if (sendInt64(fd, receiving_time) == -1) {
        reportDebug("Failed to send receiving_time.", 1);
        return -1;
    }

    if (sendInt64(fd, sending_time) == -1) {
        reportDebug("Failed to send sending_time.", 1);
        return -1;
    }

    if (sendInt64(fd, encoding_time) == -1) {
        reportDebug("Failed to send encoding_time.", 1);
        return -1;
    }

    if (sendInt64(fd, duration) == -1) {
        reportDebug("Failed to send duration.", 1);
        return -1;
    }

    if (sendInt64(fd, time_left) == -1) {
        reportDebug("Failed to send timeout.", 1);
        return -1;
    }

    if (sendAdrressStruct(fd, address) == -1) {
        reportDebug("Failed to send source address.", 1);
        return -1;
    }

    if (sendAdrressStruct(fd, src_address) == -1) {
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

    if (sendString(fd, quality) == -1) {
        reportDebug("Failed to send quality.", 1);
        return -1;
    }

    if (sendInt64(fd, run_args.size()) == -1) {
        reportDebug("Failed to send number of arguments.", 1);
        return -1;
    }

    for (u_int64_t i = 0; i < run_args.size(); ++i) {
        try {
        if (sendString(fd, run_args.at(i))) {
            reportDebug("Failed to send argument.", 1);
            return -1;
        }
        } catch(std::out_of_range) {
            reportDebug("Failed to send argument.", 1);
            return -1;
        }
    }


        RESPONSE_T resp;
    if (receiveResponse(fd, resp) == -1) {
        reportDebug("Failed to confirm chunk", 1);
        return -1;
    }

    if (resp != RESPONSE_T::ACK_FREE) {
        reportDebug("Failed to confirm chunk", 1);
        return -1;
    }

    return 0;
}

int64_t TransferInfo::receive(int64_t fd) {
    int64_t args_count;
    if (receiveInt64(fd, chunk_size) == -1) {
        reportDebug("Failed to receive size.", 1);
        return -1;
    }

     if (receiveInt64(fd, sent_times) == -1) {
        reportDebug("Failed to receive times sent.", 1);
        return -1;
    }

     if (receiveInt64(fd, receiving_time) == -1){
         reportDebug("Failed to receive receiving time.", 1);
         return -1;
     }

     if (receiveInt64(fd, sending_time) == -1){
         reportDebug("Failed to receive sending time.", 1);
         return -1;
     }

     if (receiveInt64(fd, encoding_time) == -1){
         reportDebug("Failed to receive encoding time.", 1);
         return -1;
     }

     if (receiveInt64(fd, duration) == -1){
         reportDebug("Failed to receive chunk duration.", 1);
         return -1;
     }

     if (receiveInt64(fd, time_left) == -1){
         reportDebug("Failed to receive chunk timeout.", 1);
         return -1;
     }

    struct sockaddr_storage srca;

    if (receiveAddressStruct(fd, srca) == -1) {
        reportDebug("Failed to receive address.", 1);
        return -1;
    }
    address = srca;

    if (receiveAddressStruct(fd, srca) == -1) {
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
        reportDebug("Failed to receive timestamp.", 1);
        return -1;
    }

    if ((quality = receiveString(fd)).empty()) {
        reportDebug("Failed to receive timestamp.", 1);
        return -1;
    }

    if (receiveInt64(fd, args_count) == -1) {
        reportDebug("Failed to receive number of arguments.", 1);
        return -1;
    }

    run_args.clear();
    for (int i = 0; i < args_count; ++i) {
        std::string arg;
        if ((arg = receiveString(fd)).empty()) {
            reportDebug("Failed to receive argument.", 1);
            return -1;
        }
        run_args.push_back(arg);
    }

    RESPONSE_T resp = RESPONSE_T::ACK_FREE;
    if (sendResponse(fd, resp) == -1) {
        reportDebug("Failed to send confirmation.", 1);
        return -1;
    }

    return 0;
}

void TransferInfo::init(int64_t size,
                        std::string ji, std::string n, std::string oe, std::string de,
                        std::string p, std::string oc) {
    time_left = DATA->config.intValues.at("COMPUTATION_TIMEOUT");
    tries_left = DATA->config.intValues.at("TRIES_BEFORE_RESEND");
    sending_time = 0;
    receiving_time = 0;
    sent_times = 0;
    encoding_time = 0;
    duration = 0;
    tries_sent = 0;
    chunk_size = size;
    start = 0.0;
    job_id = ji;
    name = n;
    original_extension = oe;
    desired_extension = de;
    path = p;
    output_codec = oc;
    timestamp = utilities::getTimestamp();
    assigned = false;
    run_args.push_back(
                DATA->config.getStringValue("FFMPEG_LOCATION"));
    run_args.push_back("-i");
    run_args.push_back("IN");
    run_args.push_back("-c:v");
    run_args.push_back(output_codec);
    run_args.push_back("-preset");
    run_args.push_back(DATA->config.getStringValue("QUALITY"));
    run_args.push_back("-c:a");
    run_args.push_back("copy");
    run_args.push_back("-nostdin");
    run_args.push_back("-qp");
    run_args.push_back("0");
    run_args.push_back("OUT");
}

std::string TransferInfo::getInfo() {
    std::stringstream ss;
    ss << "----------------------------------" << std::endl;
    ss << "Name: " << name << std::endl;
    ss << "Size: " << chunk_size / 1024 << " kiloBytes" << std::endl;
    ss << "Times sent: " << sent_times << std::endl;
    ss << "Sending time: " << sending_time << std::endl;
    ss << "Receiving time: " << receiving_time << std::endl;
    ss << "Encoding time: " << encoding_time << std::endl;
    ss << "Time per kB: " << time_per_kb << std::endl;
    ss << "Encoded by: " << MyAddr(address).get() << std::endl;
    ss << "---------------------------------" << std::endl;
    return ss.str();
}

std::string TransferInfo::getCSV() {
    std::stringstream ss;
    ss << name << ", ";
    ss << sent_times << ", ";
    ss << sending_time << ", ";
    ss << receiving_time << ", ";
    ss << encoding_time << ", ";
    ss << "Encoded by: " << MyAddr(address).get() << std::endl;
    return ss.str();
}

std::string TransferInfo::toString() {
    return ("TI_" + name + job_id);
}

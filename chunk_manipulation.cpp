#include "defines.h"
#include "include_list.h"
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
using namespace utilities;

void chunkSendRoutine(NetworkHandler *net_handler) {
    TransferInfo *ti;
    NeighborInfo *ngh;
    while (true) {
        unique_lock<mutex> lck(DATA->m_data.send_mtx, defer_lock);
        lck.lock();
        while (DATA->m_data.send_deq_used || !DATA->chunks_to_send.size()) {
            DATA->m_data.send_cond.wait(lck);
        }
        DATA->m_data.send_deq_used = true;
        ti = DATA->chunks_to_send.front();
        DATA->chunks_to_send.pop_front();
        DATA->m_data.send_deq_used = false;
        lck.unlock();
        DATA->m_data.send_cond.notify_one();
        if (!ti->addressed) {
            if (net_handler->getFreeNeighbor(ngh) == 0) {
                reportError("No free neighbor!");
                pushChunkSend(ti);
                //delete ti;
                sleep(5);
                continue;
            }
            int sock = net_handler->checkNeighbor(ngh->address);
            ti->address = ngh->address;
            //TODO get my address
            getHostAddr(ti->src_address, sock);
            ((struct sockaddr_in*) &ti->src_address)->sin_port =
                    htons(DATA->config.getValue("LISTENING_PORT"));
            net_handler->spawnOutgoingConnection(ngh->address, sock,
            { PING_PEER, DISTRIBUTE_PEER }, true, (void *) ti);
        } else {
            int sock = net_handler->checkNeighbor(ti->src_address);
            net_handler->spawnOutgoingConnection(ti->src_address, sock,
            { PING_PEER, RETURN_PEER }, true, (void *) ti);
            DATA->chunks_received.erase(ti->getHash());
        }
    }
}

void chunkProcessRoutine() {
    TransferInfo *ti;
    while (1) {
        unique_lock<mutex> lck(DATA->m_data.chunk_mtx, defer_lock);
        lck.lock();
        while (DATA->m_data.process_deq_used ||
               !DATA->chunks_to_encode.size() ||
               DATA->state.working)
            DATA->m_data.chunk_cond.wait(lck);
        DATA->m_data.process_deq_used = true;
        ti = DATA->chunks_to_encode.front();
        encodeChunk(ti);
        DATA->chunks_to_encode.pop_front();
        DATA->m_data.process_deq_used = false;
        lck.unlock();
        DATA->m_data.chunk_cond.notify_one();
    }
}

void processReturnedChunk(TransferInfo *ti,
                          NetworkHandler *handler, VideoState *state) {
    utilities::rmFile(DATA->config.working_dir + "/" + ti->job_id +
              "/" + ti->name + ti->original_extension);
    DATA->periodic_listeners.erase(ti->getHash());
    DATA->waiting_chunks.erase(ti->getHash());
    unique_lock<mutex> lck(DATA->m_data.send_mtx, defer_lock);
    lck.lock();
    while (DATA->m_data.send_deq_used) {
        DATA->m_data.send_cond.wait(lck);
    }
    DATA->m_data.send_deq_used = true;
    for (auto it = DATA->chunks_to_send.begin();
         it != DATA->chunks_to_send.end(); ++it) {
        if ((*it)->getHash() == ti->getHash()) {
            DATA->chunks_to_send.erase(it);
            DATA->state.to_send--;
            break;
        }
    }
    DATA->m_data.send_deq_used = false;
    lck.unlock();
    DATA->m_data.send_cond.notify_one();
    int comp_time = atoi(utilities::getTimestamp().c_str())
        - atoi(ti->timestamp.c_str());
    NeighborInfo *ngh = handler->getNeighborInfo(ti->address);
    ngh->overall_time += comp_time;
    ngh->processed_chunks++;
    ngh->quality = ngh->overall_time / ngh->processed_chunks;
    reportDebug(MyAddr(ti->address).get() +
              " new quality: " + utilities::m_itoa(ngh->quality), 3);
    MSG_T type = DEBUG;
    if (!--DATA->state.to_recv) {
        type = SUCCESS;
    }
    DATA->io_data.info_handler.updateAt(state->msgIndex,
                        utilities::formatString(
                        "processed chunks:",
                        utilities::m_itoa(++state->processed_chunks) +
                        "/" + utilities::m_itoa(state->c_chunks)), type);
}

int VideoState::split() {
    DATA->state.working = true;
    processed_chunks = false;
    std::string out, err;
    double sum = 0.0;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH], chunk_id[BUF_LENGTH], current[BUF_LENGTH], msg[BUF_LENGTH], cmd[BUF_LENGTH];
    char dir_name[BUF_LENGTH];
    sprintf(dir_name, "job_%s", utilities::getTimestamp().c_str());
    job_id = std::string(dir_name);
    std::string path(dir_location + std::string("/") + job_id);
    reportStatus(path);
    if (prepareDir(path, true) == -1) {
        reportError("Failed to create the job directory.");
        return (-1);
    }

    mins = secs_per_chunk / 60;
    secs = secs_per_chunk % 60;
    snprintf(chunk_duration, BUF_LENGTH, "00:%02d:%02d", mins, secs);
    reportStatus("Splitting file: " + finfo.fpath);
    DATA->state.to_recv = c_chunks;
    std::string infoMsg = utilities::formatString("processed chunks: ", "0/" + utilities::m_itoa(c_chunks));
    msgIndex = DATA->io_data.info_handler.add(infoMsg, DEBUG);
    DATA->io_data.info_handler.print();
    for (unsigned int i = 0; i < c_chunks; ++i) {
        double percent = (double) i / c_chunks;

        printProgress(percent);
        elapsed = i * secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        snprintf(current, BUF_LENGTH, "%02d:%02d:%02d", hours, mins, secs);
        snprintf(output, BUF_LENGTH, "%s/%s/%03d_splitted%s",
                 //TODO check if exists, HARDCODE
                 dir_location.c_str(), job_id.c_str(), i, ".avi");
        snprintf(chunk_id, BUF_LENGTH, "%03d_splitted", i);
        snprintf(cmd, BUF_LENGTH, "ffmpeg");
        cursToStatus();
        sum += Measured<>::exec_measure(runExternal, out, err, cmd, 12, cmd,
                    "-ss", current,
                    "-i", finfo.fpath.c_str(),
                    "-v", "quiet",
                    "-c", "copy",
                    "-t", chunk_duration,
                    output);
        if (err.find("Conversion failed") != std::string::npos) {
            sprintf(msg, "%s %s %s %s %s %s %s %s\n", "ffmpeg",
                "-i", finfo.fpath.c_str(),
                "-ss", current,
                "-t", chunk_duration,
                output);
            reportError(msg);
            abort();
            return (-1);
        }
        TransferInfo *ti = new TransferInfo(getFileSize(output),
                                            job_id, chunk_id, finfo.extension, o_format,
                                            std::string(output), o_codec);
        DATA->state.to_send++;
        pushChunkSend(ti);
    }
    printProgress(1);
    reportSuccess("File successfuly splitted.");
    reportTime("/splitting.sp", sum / 1000);
    clearProgress();
    return (0);
}

void VideoState::abort() {
    clearProgress();
    DATA->state.working = false;
}

void pushChunk(TransferInfo *ti, std::mutex &mtx, std::condition_variable &cond,
               bool &ctrl_var, std::deque<TransferInfo *> &queue) {
    unique_lock<mutex> lck(mtx, defer_lock);

    lck.lock();
    while (ctrl_var)
        cond.wait(lck);
    ctrl_var = true;
    queue.push_back(ti);
    ctrl_var = false;
    lck.unlock();
    cond.notify_one();
}

void pushChunkProcess(TransferInfo * ti) {
    pushChunk(ti, DATA->m_data.chunk_mtx, DATA->m_data.chunk_cond,
              DATA->m_data.process_deq_used, DATA->chunks_to_encode);
}

void pushChunkSend(TransferInfo * ti) {
    pushChunk(ti, DATA->m_data.send_mtx, DATA->m_data.send_cond,
              DATA->m_data.send_deq_used, DATA->chunks_to_send);
}

int VideoState::join() {
    std::string out, err, list_loc(DATA->config.working_dir + "/received/" + job_id + "/join_list.txt"),
            output(DATA->config.working_dir + "/" + finfo.basename + "_output" + o_format);
    std::string file, file_item;
    ofstream ofs(list_loc);
    char fn[BUF_LENGTH], cmd[BUF_LENGTH];
    long sum_size = 0;
    double duration = 0;
    unlink(output.c_str());
    errno = 0;
    reportStatus("Joining the file: " + output);
    snprintf(cmd, BUF_LENGTH, "ffmpeg");
    for (unsigned int i = 0; i < c_chunks; ++i) {
        snprintf(fn, BUF_LENGTH, "%03d_splitted", i);
        //HARDCODE
        file = DATA->config.working_dir + "/received/" + job_id + "/" + fn + o_format;
//        if (!utilities::getFileSize())
        //check existence
        file_item = "file '" + file + "'";
        sum_size += utilities::getFileSize(file);
        ofs << file_item << endl;
    }
    ofs.flush();
    ofs.close();
    thread thr(reportFileProgress, output, sum_size);
    duration = Measured<>::exec_measure(runExternal, out, err, cmd, 8, cmd,
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    output.c_str());
    if (err.find("failed") != std::string::npos) {
        thr.detach();
        reportError(output + ": Error while joining file.");
        reportError(err);
        clearProgress();
        return (-1);
    }
    thr.join();
    printProgress(1);
    reportSuccess("Succesfully joined.");
    reportTime("/joining.j", duration / 1000);
    clearProgress();
    utilities::rmrDir(DATA->config.working_dir + "/received/", true);
    DATA->state.working = false;
    return (0);
}


void VideoState::printVideoState() {
    DATA->io_data.info_handler.print();
}

void VideoState::loadFileInfo(finfo_t &finfo) {
    this->finfo = finfo;
    changeChunkSize(DATA->config.getValue("CHUNK_SIZE"));
    DATA->io_data.info_handler.clear();
    if (!finfo.fpath.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("File:",
                                                               finfo.fpath), PLAIN);
    }
    if (!finfo.codec.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Codec:",
                                                               finfo.codec), PLAIN);
    }
    if (finfo.bitrate) {
        DATA->io_data.info_handler.add(utilities::formatString("Bitrate:",
                                                               utilities::m_itoa(finfo.bitrate)), PLAIN);
    }
    if (finfo.duration) {
        DATA->io_data.info_handler.add(utilities::formatString("Duration:",
                                                               utilities::m_itoa(finfo.duration)), PLAIN);
        }
    if (finfo.fsize) {
        DATA->io_data.info_handler.add(utilities::formatString("File size:",
                                                               utilities::m_itoa(finfo.fsize)), PLAIN);
    }
    if (chunk_size) {
        DATA->io_data.info_handler.add(utilities::formatString("Chnuk size:",
                                                               utilities::m_itoa(
                                                                   DATA->config.getValue("CHUNK_SIZE"))), PLAIN);
    }
    if (!o_codec.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Output codec:",
                                                               o_codec), PLAIN);
    }
    if (!o_format.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Output extension:",
                                                               o_format), PLAIN);
    }
}

void VideoState::resetFileInfo() {
    finfo.fpath = "";
    finfo.codec = "";
    finfo.bitrate = 0.0;
    finfo.duration = 0.0;
    finfo.fsize = 0.0;
    secs_per_chunk = 0;
    c_chunks = 0;
}

void VideoState::changeChunkSize(size_t nsize) {
    chunk_size = nsize;
    secs_per_chunk = chunk_size  / finfo.bitrate;
    c_chunks = (((size_t) finfo.fsize) / chunk_size) + 1;
}

HistoryStorage::HistoryStorage(const std::string &fn): filename(fn), c_index(0) {
    ifstream in(fn);
    std::string line;
    while (in.good()) {
        getline(in, line);
        if (!line.empty())
            history.push_back(line);
    }
    c_index = history.size() ;
}

void HistoryStorage::next() {
    if (c_index < history.size() )
        ++c_index;
    else
        c_index = 0;
}

void HistoryStorage::prev() {
    if (c_index)
        --c_index;
    else
        c_index = history.size() - 1;
}

void HistoryStorage::save(string line) {
    if (line.empty())
        return;
    history.push_back(line);
    c_index = history.size();
}

std::string &HistoryStorage::getCurrent() {
    return history.at(c_index);
}

void HistoryStorage::write() {
    ofstream out(filename);
    for(string &s : history)
        out << s << endl;
    out.close();
}

string MyAddr::get() {
    std::stringstream ss;
    ss << "Address: " << addr << " : " << port;
    return (ss.str());
}

void MyAddr::print() {
    reportStatus(get());
}

MyAddr::MyAddr(struct sockaddr_storage &address) {
    char adr4[sizeof (struct sockaddr_in)], adr6[sizeof (struct sockaddr_in6)];
    if (((struct sockaddr_in *)(&address))->sin_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *) &address;
        inet_ntop(AF_INET, &addr->sin_addr,
              adr4, sizeof(struct sockaddr_in));
        this->addr = std::string(adr4);
        port = ntohs(addr->sin_port);
    } else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &address;
        inet_ntop(AF_INET6, &addr->sin6_addr,
              adr6, sizeof(struct sockaddr_in6));
       this->addr = std::string(adr6);
        port = ntohs(addr->sin6_port);
    }
}

//the indexing is not permanent
int WindowPrinter::add(string str, MSG_T type) {
    int retval;
    DATA->m_data.report_mtx.lock();
    if ((direction == DOWN) || (direction == STATIC)) {
        q.push_back(make_pair(str, type));
        retval = q.size() - 1;
    } else {
        q.push_front(make_pair(str, type));
        retval = 0;
    }
    DATA->m_data.report_mtx.unlock();
    return retval;
}

void WindowPrinter::changeWin(WINDOW *nwin) {
    win = nwin;
}

void WindowPrinter::clear() {
    q.clear();
}

void WindowPrinter::updateAt(int idx, std::string value, MSG_T type) {
    q.at(idx) = std::make_pair<>(value, type);
    print();
}

void WindowPrinter::print() {
    unique_lock<mutex> lck(DATA->m_data.O_mtx, defer_lock);

    lck.lock();
    while (DATA->m_data.using_O)
        DATA->m_data.IO_cond.wait(lck);
    DATA->m_data.using_O = true;
    DATA->m_data.report_mtx.lock();
    wbkgd(win, COLOR_PAIR(BG));
    wmove(win, 0, 0);
    werase(win);
    int y, col;
    if (start == BOTTOM)
        y = DATA->config.intValues.at("STATUS_LENGTH") - 1;
    else
        y = 0;
    bool first = true;
    deque<printable_pair_T>::iterator s, e;
    if (direction == UP) {
        s = q.begin();
    } else {
        s = q.size() > (unsigned) DATA->config.intValues.at("STATUS_LENGTH") ?
                    q.end() - DATA->config.intValues.at("STATUS_LENGTH") +1 : q.begin();
    }
    e = q.end();
    while (s != e) {
        auto a = *s++;
        if (first && bolded)
            wattron(win, A_BOLD);
        col = DEFAULT;
        if (a.second == ERROR)
            col = RED;
        if (a.second == SUCCESS)
            col = GREEN;
        if (a.second == DEBUG) {
            col = BLUE;
        }
        if (col != DEFAULT)
            wattron(win, COLOR_PAIR(col));
        if (start == BOTTOM)
            --y;
        else
            ++y;
        mvwprintw(win, y, 1, "%s\n", a.first.c_str());
        wrefresh(win);
        if (col != DEFAULT)
            wattroff(win,	COLOR_PAIR(col));
        if (first && bolded) {
            first = false;
            wattroff(win, A_BOLD);
        }
    }
    refresh();
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(win);
    DATA->m_data.report_mtx.unlock();
    DATA->m_data.using_O = false;
    lck.unlock();
    DATA->m_data.IO_cond.notify_one();
}

void NeighborInfo::printInfo() {
    MyAddr mad(address);
    reportSuccess(mad.get() + "; " + m_itoa(intervals));
}

void NeighborInfo::invoke(NetworkHandler &net_handler) {
    int sock;
    if (!--intervals) {
        sock = net_handler.checkNeighbor(address);
        net_handler.spawnOutgoingConnection(address, sock, { PING_PEER }, true, nullptr);
    }
}

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
            reportSuccess("Time is up, neighbor alive.");
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

string NeighborInfo::getHash() {
    std::string hash(storage2addr(address) +
                     m_itoa(((struct sockaddr_in *)&address)->sin_port));
    return hash;
}

string TransferInfo::getHash() {
    return (name + job_id);
}

int Configuration::getValue(string key) {
    try {
        return intValues.at(key);
    } catch (std::out_of_range e) {
        return 0;
    }
}

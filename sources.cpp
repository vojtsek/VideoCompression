#include "defines.h"
#include "common.h"

#include <string>
#include <fstream>
#include <utility>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <unistd.h>
#include <arpa/inet.h>
#include <curses.h>

using namespace std;
using namespace common;

int VideoState::split() {
    string out, err;
    double sum = 0.0;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH], current[BUF_LENGTH], msg[BUF_LENGTH], cmd[BUF_LENGTH];

    if (prepareDir(dir_location) == -1) {
        reportError("Failed to create the job directory.");
        return (-1);
    }
    mins = secs_per_chunk / 60;
    secs = secs_per_chunk % 60;
    snprintf(chunk_duration, BUF_LENGTH, "00:%02d:%02d", mins, secs);
    reportStatus("Splitting file: " + finfo.fpath);
    for (unsigned int i = 0; i < c_chunks; ++i) {
        double percent = (double) i / c_chunks;
        printProgress(percent);
        elapsed = i * secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        snprintf(current, BUF_LENGTH, "%02d:%02d:%02d", hours, mins, secs);
        snprintf(output, BUF_LENGTH, "%s/%03d_splitted.avi", dir_location.c_str(), i);
        snprintf(cmd, BUF_LENGTH, "ffmpeg");
        cursToStatus();
        sum += Measured<>::exec_measure(runExternal, out, err, cmd, 12, cmd,
                    "-ss", current,
                    "-i", finfo.fpath.c_str(),
                    "-v", "quiet",
                    "-c", "copy",
                    "-t", chunk_duration,
                    output);
        if (err.find("Conversion failed") != string::npos) {
            sprintf(msg, "%s %s %s %s %s %s %s %s\n", "ffmpeg",
                "-i", finfo.fpath.c_str(),
                "-ss", current,
                "-t", chunk_duration,
                output);
            reportError(msg);
            clearProgress();
            return (-1);
        }
    }
    printProgress(1);
    reportSuccess("File successfuly splitted.");
    reportTime("/splitting.sp", sum / 1000);
    clearProgress();
    return (0);
}


int VideoState::join() {
    string out, err, list_loc(dir_location + "/join_list.txt"), output(finfo.basename + "_output." + finfo.extension);
    ofstream ofs(list_loc);
    stringstream ss;
    char file[BUF_LENGTH], cmd[BUF_LENGTH];
    long sum_size = 0;
    double duration = 0;
    unlink(output.c_str());
    errno = 0;
    reportStatus("Joining the file: " + output);
    snprintf(cmd, BUF_LENGTH, "ffmpeg");
    for (unsigned int i = 0; i < c_chunks; ++i) {
        snprintf(file, BUF_LENGTH, "file '%03d_splitted.avi'", i);
        try {
            ss.clear();
            ss.str("");
            ss << dir_location << "/" << setfill('0') << setw(3) << i << "_splitted.avi";
            sum_size += getFileSize(ss.str());
            ofs << file << endl;
        } catch (...) {} // TODO: handle exception
    }
    ofs.flush();
    ofs.close();
    thread thr(reportFileProgress, output, sum_size);
    duration = Measured<>::exec_measure(runExternal, out, err, cmd, 8, cmd,
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    output.c_str());
    if (err.find("failed") != string::npos) {
        thr.detach();
        clearProgress();
        return (-1);
    }
    thr.join();
    printProgress(1);
    reportSuccess("Succesfully joined.");
    reportTime("/joining.j", duration / 1000);
    clearProgress();
    return (0);
}


void VideoState::printVideoState() {
    char value[BUF_LENGTH];
    if (!finfo.fpath.empty())
        snprintf(value, BUF_LENGTH, "%15s%35s", "File:", finfo.fpath.c_str());
        DATA->info_handler.add(string(value), PLAIN);
    if (!finfo.codec.empty())
        snprintf(value, BUF_LENGTH, "%15s%35s", "Codec:", finfo.codec.c_str());
        DATA->info_handler.add(string(value), PLAIN);
    if (finfo.bitrate) {
        snprintf(value, BUF_LENGTH, "%15s%35f", "Bitrate:", finfo.bitrate);
        DATA->info_handler.add(string(value), PLAIN);
    }
    if (finfo.duration) {
        snprintf(value, BUF_LENGTH, "%15s%35f", "Duration:", finfo.duration);
        DATA->info_handler.add(string(value), PLAIN);
    }
    if (finfo.fsize) {
        snprintf(value, BUF_LENGTH, "%15s%35f", "Filesize:", finfo.fsize);
        DATA->info_handler.add(string(value), PLAIN);
    }
    if (chunk_size) {
        snprintf(value, BUF_LENGTH, "%15s%35d", "Chunk size:", DATA->intValues.at("CHUNK_SIZE"));
        DATA->info_handler.add(string(value), PLAIN);
    }
    DATA->info_handler.print();
}

void VideoState::loadFileInfo(finfo_t &finfo) {
    this->finfo = finfo;
    char dir_name[BUF_LENGTH];
    sprintf(dir_name, "job_%s", common::getTimestamp().c_str());
    dir_location = DATA->working_dir + "/" + std::string(dir_name);
    changeChunkSize(CHUNK_SIZE);
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

HistoryStorage::HistoryStorage(const string &fn): filename(fn), c_index(0) {
    ifstream in(fn);
    string line;
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
    stringstream ss;
    ss << "Address: " << addr << " : " << port;
    return (ss.str());
}

void MyAddr::print() {
    reportStatus(get());
}

MyAddr::MyAddr(struct sockaddr_storage &address) {
    char adr4[sizeof (struct sockaddr_in)], adr6[sizeof (struct sockaddr_in6)];
    if (((struct sockaddr *)(&address))->sa_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *) &address;
        inet_ntop(AF_INET, &addr->sin_addr,
              adr4, sizeof(struct sockaddr_in));
        this->addr = string(adr4);
        port = ntohs(addr->sin_port);
    } else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &address;
        inet_ntop(AF_INET6, &addr->sin6_addr,
              adr6, sizeof(struct sockaddr_in6));
       this->addr = string(adr6);
        port = ntohs(addr->sin6_port);
    }
}

void WindowPrinter::add(string str, MSG_T type) {
    DATA->report_mtx.lock();
    if (direction == DOWN)
        q.push_back(make_pair(str, type));
    else
        q.push_front(make_pair(str, type));
    DATA->report_mtx.unlock();
}

void WindowPrinter::changeWin(WINDOW *nwin) {
    win = nwin;
}

void WindowPrinter::print() {
    unique_lock<mutex> lck(DATA->IO_mtx, defer_lock);

    lck.lock();
    while (DATA->using_IO)
        DATA->cond.wait(lck);
    DATA->using_IO = true;
    DATA->report_mtx.lock();
    wbkgd(win, COLOR_PAIR(BG));
    wmove(win, 0, 0);
    werase(win);
    int y, col;
    if (start == BOTTOM)
        y = DATA->intValues.at("STATUS_LENGTH") - 1;
    else
        y = 0;
    bool first = true;
    deque<printable_pair_T>::iterator s, e;
    if (direction == UP) {
        s = q.begin();
    } else {
        s = q.size() > (unsigned) DATA->intValues.at("STATUS_LENGTH") ?
                    q.end() - DATA->intValues.at("STATUS_LENGTH") +1 : q.begin();
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
    DATA->report_mtx.unlock();
    DATA->using_IO = false;
    lck.unlock();
    DATA->cond.notify_one();
}

void NeighborInfo::printInfo() {
    MyAddr mad(address);
    reportStatus(mad.get());
}

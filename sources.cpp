#include "defines.h"
#include "common.h"

#include <string>
#include <fstream>
#include <utility>
#include <mutex>
#include <thread>
#include <condition_variable>

#include <unistd.h>
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
    wmove(Data::getInstance()->info_win, 0, 0);
    werase(Data::getInstance()->info_win);
    if (!finfo.fpath.empty())
        show("File:", finfo.fpath.c_str());
    if (!finfo.codec.empty())
        show("Codec:", finfo.codec.c_str());
    if (finfo.bitrate) {
        snprintf(value, BUF_LENGTH, "%f", finfo.bitrate);
        show("Bitrate:", value);
    }
    if (finfo.duration) {
        snprintf(value, BUF_LENGTH, "%f", finfo.duration);
        show("Duration:", value);
    }
    if (finfo.fsize) {
        snprintf(value, BUF_LENGTH, "%f", finfo.fsize);
        show("File size:", value);
        snprintf(value, BUF_LENGTH, "%d", c_chunks);
        show("Chunks count:", value);
    }
    if (chunk_size) {
        snprintf(value, BUF_LENGTH, "%d", chunk_size / 1024);
        show("Chunk size:", value);
    }
    wbkgd(Data::getInstance()->info_win, COLOR_PAIR(INVERTED));
    attron(A_BOLD);
    attron(COLOR_PAIR(BLUE));
    show("Output format:", o_format.c_str());
    show("Output codec:", o_codec.c_str());
    attroff(COLOR_PAIR(BLUE));
    attroff(A_BOLD);
    wborder(Data::getInstance()->info_win, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(Data::getInstance()->info_win);
}

void VideoState::loadFileInfo(finfo_t &finfo) {
    this->finfo = finfo;
    char dir_name[BUF_LENGTH];
    sprintf(dir_name, "job_%s", common::getTimestamp().c_str());
    dir_location = Data::getInstance()->working_dir + "/" + std::string(dir_name);
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

void StatusInfo::add(status_pairT &msg) {
    Data::getInstance()->report_mtx.lock();
    q.push_front(make_pair(msg.first, msg.second));
    //if (q.size() > STATUS_LENGTH)
      //  q.pop_back();
    Data::getInstance()->report_mtx.unlock();
}

void StatusInfo::print() {
    unique_lock<mutex> lck(Data::getInstance()->IO_mtx, defer_lock);

    lck.lock();
    while (Data::getInstance()->using_IO)
        Data::getInstance()->cond.wait(lck);
    Data::getInstance()->using_IO = true;
    Data::getInstance()->report_mtx.lock();
    WINDOW *win = Data::getInstance()->status_win;
    int y = Data::getInstance()->status_length - 1, col;
    bool first = true;
    for (status_pairT &a : q) {
        if (first)
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
        mvwprintw(win, --y, 0, "%s\n", a.first.c_str());
        wrefresh(win);
        if (col != DEFAULT)
            wattroff(win,	COLOR_PAIR(col));
        if (first) {
            first = false;
            wattroff(win, A_BOLD);
        }
    }
    refresh();
    wrefresh(win);
    Data::getInstance()->report_mtx.unlock();
    Data::getInstance()->using_IO = false;
    lck.unlock();
    Data::getInstance()->cond.notify_one();
}

void NeighborInfo::printInfo() {
    reportStatus(common::addr2str(address));
}

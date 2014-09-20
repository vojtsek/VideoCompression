#include <string>
#include <curses.h>
#include <fstream>
#include <utility>
#include <mutex>
#include <condition_variable>
#include "defines.h"
#include "common.h"

using namespace std;

void State::printState() {
    char value[BUF_LENGTH];
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
    attron(A_BOLD);
    attron(COLOR_PAIR(BLUE));
    show("Output format:", o_format.c_str());
    show("Output codec:", o_codec.c_str());
    attroff(COLOR_PAIR(BLUE));
    attroff(A_BOLD);
}

void State::loadFileInfo(finfo_t &finfo) {
    this->finfo = finfo;
    char dir_name[BUF_LENGTH];
    sprintf(dir_name, "job_%s", common::getTimestamp().c_str());
    dir_location = configuration["WD_PREFIX"] + "/" + std::string(dir_name);
    changeChunkSize(CHUNK_SIZE);
}

void State::resetFileInfo() {
    finfo.fpath = "";
    finfo.codec = "";
    finfo.bitrate = 0.0;
    finfo.duration = 0.0;
    finfo.fsize = 0.0;
    secs_per_chunk = 0;
    c_chunks = 0;
}

void State::changeChunkSize(size_t nsize) {
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

void StatusInfo::add(status_pairT msg) {
    q.push_front(make_pair(msg.first, msg.second));
    if (q.size() > 3)
        q.pop_back();
}

void StatusInfo::print() {
    unique_lock<mutex> lck(Data::getInstance()->input_mtx, defer_lock);

    lck.lock();
    while (Data::getInstance()->reading_input)
        Data::getInstance()->cond.wait(lck);
    Data::getInstance()->reading_input = true;
    common::cursToStatus();
    int x, y, col;
    bool first = true;
    getyx(stdscr, y, x);
    for (status_pairT a : q) {
        if (first)
            attron(A_BOLD);
        col = DEFAULT;
        if (a.second == ERROR)
            col = RED;
        if (a.second == SUCCESS)
            col = GREEN;
        if (col != DEFAULT)
        attron(COLOR_PAIR(col));
        printw("%s\n", a.first.c_str());
        move(--y, x);
        if (col != DEFAULT)
        attroff(	COLOR_PAIR(col));
        if (first) {
            first = false;
            attroff(A_BOLD);
        }
    }
    refresh();
    Data::getInstance()->reading_input = false;
    lck.unlock();
    Data::getInstance()->cond.notify_one();
}

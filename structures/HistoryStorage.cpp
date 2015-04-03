#include "headers/defines.h"
#include "headers/include_list.h"

#include <fstream>


HistoryStorage::HistoryStorage(const std::string &fn): filename(fn), c_index(0) {
    std::ifstream in(fn);
    std::string line;
    // reads the file line by line and saves
    while (in.good()) {
        getline(in, line);
        if (!line.empty()) {
            history.push_back(line);
        }
    }
    // index begins "one line behind"
    c_index = history.size();
}

void HistoryStorage::next() {
    // adjusts the index accordingly
    if (c_index < history.size() - 1) {
        ++c_index;
    } else {
        c_index = 0;
    }
}

void HistoryStorage::prev() {
    // adjusts the index accordingly
    if (c_index) {
        --c_index;
    } else {
        c_index = history.size() - 1;
    }
}

void HistoryStorage::save(std::string line) {
    if (line.empty()) {
        return;
    }
    // saves the line to buffer
    history.push_back(line);
    c_index = history.size();
}

std::string &HistoryStorage::getCurrent() {
    return history.at(c_index);
}

void HistoryStorage::write() {
    // writes to a file
    std::ofstream out(filename);
    for(std::string &s : history)
        out << s << std::endl;
    out.close();
}

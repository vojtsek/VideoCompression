#include "defines.h"
#include "include_list.h"

#include <fstream>


HistoryStorage::HistoryStorage(const std::string &fn): filename(fn), c_index(0) {
    std::ifstream in(fn);
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

void HistoryStorage::save(std::string line) {
    if (line.empty())
        return;
    history.push_back(line);
    c_index = history.size();
}

std::string &HistoryStorage::getCurrent() {
    return history.at(c_index);
}

void HistoryStorage::write() {
    std::ofstream out(filename);
    for(std::string &s : history)
        out << s << std::endl;
    out.close();
}

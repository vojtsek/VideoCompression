#include <string>
#include <fstream>
#include"defines.h"

using namespace std;

HistoryStorage::HistoryStorage(const string &fn): filename(fn), c_index(0) {
    ifstream in(fn);
    string line;
    while (in.good()) {
        getline(in, line);
        history.push_back(line);
    }
    c_index = history.size() ;
}

void HistoryStorage::next() {
    if (c_index < history.size() )
        ++c_index;
}

void HistoryStorage::prev() {
    if (c_index)
        --c_index;
}

void HistoryStorage::save(string line) {
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
}

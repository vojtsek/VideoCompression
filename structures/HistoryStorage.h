#ifndef HISTORYSTORAGE_H
#define HISTORYSTORAGE_H


class HistoryStorage {
    std::vector<std::string> history;
    std::string filename;
    uint32_t c_index;
public:
    HistoryStorage(const std::string &fn);
    void prev();
    void next();
    void save(std::string line);
    void write();
    std::string &getCurrent();
};


#endif // HISTORYSTORAGE_H

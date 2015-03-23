#define _FILE_OFFSET_BITS 64
#include "headers/include_list.h"
#include "headers/handle_IO.h"
#include "headers/commands.h"
#include "headers/defines.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <curses.h>
#include <deque>
#include <thread>
#include <string.h>
#include <sys/stat.h>


using namespace std;

std::shared_ptr<Data> Data::inst = nullptr;

int32_t utilities::acceptCmd(cmd_storage_t &cmds) {
    wchar_t c;
    std::unique_lock<std::mutex> lck(DATA->m_data.I_mtx, std::defer_lock);
    do {
        lck.lock();
        while (DATA->m_data.using_I)
            DATA->m_data.IO_cond.wait(lck);
        DATA->m_data.using_I = true;
        c = getch();
        DATA->m_data.using_I = false;
        lck.unlock();
        DATA->m_data.IO_cond.notify_one();
        usleep(10000);
    } while(c == ERR);
    if (c == KEY_F(12)) {
        return (1);
    }
    thread thr ([&]() {
        try {
            cmds.at(Data::getCmdMapping().at(c))->execute();
        } catch (out_of_range) {
            cmds[DEFCMD]->execute();
        }
    });
    thr.detach();
    usleep(10000);
    return(0);
}

void utilities::listCmds() {
    printw("List of available commands:\n");
    show("help", "[command_name]");
    show("show", "parametres|jobs|neighbors");
    show("load", "/path/to/file");
    show("set", "[parameter=new_value[,...]]");
    show("start", "");
    show("quit", "");
}

void clearNlines(int32_t n) {
    int32_t orig_x, orig_y, x, y;
    getyx(stdscr, y, x);
    orig_x = x;
    orig_y = y;
    while(n--) {
        move(y++, 0);
        clrtoeol();
    }
    move(orig_y, orig_x);
}


string utilities::m_itoa(int32_t n) {
    std::string res;
    int32_t nn;
    bool negative = false;
    if (n < 0) {
        negative = true;
        n *= -1;
    }
    if (n == 0) {
        return std::string("0");
    }
    while(n > 0) {
        nn = n % 10;
        n /= 10;
        res.push_back('0' + nn);
    }
    reverse(res.begin(), res.end());
    if (negative) {
        return "-" + res;
    }
    return res;
}

int32_t utilities::computeDuration(std::string t1, std::string t2) {
    return (atoi(t1.c_str()) - atoi(t2.c_str()));
}

vector<string> utilities::extract(const std::string text, const std::string from, int32_t count) {
    vector<string> result;
    std::string word;
    std::stringstream ss(text);
    bool start = false;
    while (ss.good()) {
        ss >> word;
        if ((!start) && ((from == "") || (word == from)))
            start = true;
        if (start) {
            result.push_back(word);
            --count;
            if (!count)
                break;
        }
    }

    return result;
}

std::string utilities::formatString(std::string str1, std::string str2) {
    char value[BUF_LENGTH];
    snprintf(value, BUF_LENGTH, "%20s%35s", str1.c_str(), str2.c_str());
    return std::string(value);
}

std::string utilities::getTimestamp() {
    char stamp[16];
    sprintf(stamp, "%d", (int) time(NULL));
    return std::string(stamp);
}

bool utilities::isAcceptable(char c) {
    vector<char> acc = {'/', '.', '_', ' ', '=', '-'};
    if ((!isgraph(c)) || (iscntrl(c)))
        return false;
    return ((isalnum(c)) || (find(acc.begin(), acc.end(), c) != acc.end()));
}

bool utilities::knownCodec(const std::string &cod) {
    vector<string> know = Data::getKnownCodecs();
    for (string &c : know) {
        if (c == cod)
            return true;
    }
    return false;
}

bool utilities::knownFormat(const std::string &format) {
    vector<string> know = Data::getKnownFormats();
    for (string &f : know) {
        if (f == format)
            return true;
    }
    return false;
}

vector<std::string> utilities::split(const std::string &content, char sep) {
    size_t pos;
    vector<string> result;
    std::string remaining(content);
    while((pos = remaining.find(sep)) != std::string::npos) {
        result.push_back(remaining.substr(0, pos));
        remaining = remaining.substr(pos + 1, remaining.length());
    }
    result.push_back(remaining);
    return result;
}

int32_t Configuration::getValue(string key) {
    try {
        return intValues.at(key);
    } catch (std::out_of_range e) {
        return 0;
    }
}

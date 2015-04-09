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
#include <chrono>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
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

int64_t utilities::acceptCmd(cmd_storage_t &cmds) {
    wchar_t c;
    std::unique_lock<std::mutex> lck(DATA->m_data.I_mtx, std::defer_lock);

    do {
        lck.lock();
        while (DATA->m_data.using_I)
            DATA->m_data.IO_cond.wait(lck);
        DATA->m_data.using_I = true;
        // non blocking mode is set, waits for key
        c = getch();
        DATA->m_data.using_I = false;
        lck.unlock();
        DATA->m_data.IO_cond.notify_one();
        usleep(10000);
        // repeats while no input is available
    } while(c == ERR);
    // F12 terminates
    if (c == KEY_F(12)) {
        return (1);
    }
    // spawns the command
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

void utilities::printOverallState(VideoState *state) {
    MSG_T type = PLAIN;
    // all chunks were delivered
    if ((state->processed_chunks == state->chunk_count) &&
    (state->chunk_count != 0)) {
        type = SUCCESS;
    }
    DATA->io_data.info_handler.clear();
    // adds the messages
    DATA->io_data.info_handler.add(utilities::formatString(
                                       "processed chunks:",
                                       utilities::m_itoa(state->processed_chunks) +
                                       "/" + utilities::m_itoa(state->chunk_count)), type);
    DATA->io_data.info_handler.add("Chunks that I am processing: ", DEBUG);
    // chunks being processed
    for (const auto &c : DATA->chunks_received.getValues()) {
        DATA->io_data.info_handler.add(c->name + " (" +
                                       utilities::m_itoa(c->chunk_size) + "B); " +
                                       MyAddr(c->src_address).get(), PLAIN);
    }

    DATA->io_data.info_handler.add("Chunks to send: ", DEBUG);
    for (const auto &ti : DATA->chunks_to_send.getValues()) {
        DATA->io_data.info_handler.add(ti->name, PLAIN);
    }
    DATA->io_data.info_handler.print();
}

std::string utilities::m_itoa(int64_t n) {
    std::string res;
    int64_t nn;
    bool negative = false;
    // handle negative values
    if (n < 0) {
        negative = true;
        n *= -1;
    }
    // zero case
    if (n == 0) {
        return std::string("0");
    }
    // do the conversion
    while(n > 0) {
        nn = n % 10;
        n /= 10;
        res.push_back('0' + nn);
    }
    reverse(res.begin(), res.end());

    // negatives
    if (negative) {
        return "-" + res;
    }
    return res;
}

int64_t utilities::computeDuration(std::string t1, std::string t2) {
    return (atoll(t1.c_str()) - atoll(t2.c_str()));
}

vector<string> utilities::extract(const std::string text, const std::string from, int64_t count) {
    vector<std::string> result;
    std::string word;
    std::stringstream ss(text);
    bool start = false;
    while (ss.good()) {
        ss >> word;
        // read the words until the passed value
        if ((!start) && ((from == "") ||
                         (word.find(from) != std::string::npos))) {
            start = true;
        }
        // extract given nuber of words
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
    std::chrono::milliseconds ms =
                std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    );
    return utilities::m_itoa(ms.count());
}

bool utilities::isAcceptable(char c) {
    vector<char> acc = {'/', '.', '_', ' ', '=', '-'};
    // exclude special characters
    if ((!isgraph(c)) || (iscntrl(c))) {
        return false;
    }
    // accept alphanumeric or listed characters
    return ((isalnum(c)) || (find(acc.begin(), acc.end(), c) != acc.end()));
}

bool utilities::knownCodec(const std::string &cod) {
    vector<string> know = Data::getKnownCodecs();
    // try to find current codec
    for (string &c : know) {
        if (c == cod) {
            return true;
        }
    }
    return false;
}

bool utilities::knownFormat(const std::string &format) {
    vector<string> know = Data::getKnownFormats();
    // try to find given format
    for (string &f : know) {
        if (f == format) {
            return true;
        }
    }
    return false;
}

vector<std::string> utilities::split(const std::string &content, char sep) {
    size_t pos;
    vector<string> result;
    std::string remaining(content);
    // while still some occurences of the separator
    while((pos = remaining.find(sep)) != std::string::npos) {
        // process next part
        result.push_back(remaining.substr(0, pos));
        remaining = remaining.substr(pos + 1, remaining.length());
    }
    // remaining part
    result.push_back(remaining);
    return result;
}

int64_t Configuration::getIntValue(std::string key) {
    int64_t res = 0;
    // safely returns the integer value
    try {
        res = intValues.at(key);
    } catch (std::out_of_range) {
        return 0;
    }
    return res;
}

std::string Configuration::getStringValue(std::string key) {
    std::string res("");
    // safely returns the string value
    try {
        res = strValues.at(key);
    } catch (std::out_of_range) {
        return "";
    }
    return res;
}

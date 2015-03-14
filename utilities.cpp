#define _FILE_OFFSET_BITS 64
#include "include_list.h"
#include "handle_IO.h"
#include "commands.h"
#include "defines.h"

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

int utilities::acceptCmd(cmd_storage_t &cmds) {
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
    if (c == KEY_F(12))
        return (1);
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

void clearNlines(int n) {
    int orig_x, orig_y, x, y;
    getyx(stdscr, y, x);
    orig_x = x;
    orig_y = y;
    while(n--) {
        move(y++, 0);
        clrtoeol();
    }
    move(orig_y, orig_x);
}


string utilities::m_itoa(int n) {
    std::string res;
    int nn;
    if (n == 0)
        return std::string("0");
    while(n > 0) {
        nn = n % 10;
        n /= 10;
        res.push_back('0' + nn);
    }
    reverse(res.begin(), res.end());
    return res;
}

int utilities::encodeChunk(TransferInfo *ti) {
    std::string out, err, res_dir;
    char cmd[BUF_LENGTH];
    res_dir = DATA->config.working_dir + "/processed/";
    if (prepareDir(res_dir, false) == -1) {
        reportDebug("Failed to create working dir.", 2);
        return -1;
    }
    res_dir += ti->job_id;
    if (prepareDir(res_dir, false) == -1) {
        reportDebug("Failed to create job dir.", 2);
        return -1;
    }
    std::string file_out = res_dir + "/" + ti->name + ti->desired_extension;
    std::string file_in = DATA->config.working_dir + "/to_process/" +
            ti->job_id + "/" + ti->name + ti->original_extension;
    reportDebug("Encoding: " + file_in, 2);
    snprintf(cmd, BUF_LENGTH, "/usr/bin/ffmpeg");
    int duration = Measured<>::exec_measure(runExternal, out, err, cmd, 10, cmd,
             "-i", file_in.c_str(),
             "-c:v", ti->output_codec.c_str(),
             "-preset", "ultrafast",
             "-qp", "0",
             file_out.c_str());
    if (err.find("Conversion failed") != std::string::npos) {
        reportDebug("Failed to encode chunk!", 2);
        std::ofstream os(ti->job_id + ".err");
        os << err << std::endl;
        os.flush();
        os.close();
        //should retry?
        delete ti;
        std::atomic_fetch_add(&DATA->state.can_accept, 1);
        return -1;
    }
    reportDebug("Chunk " + ti->name + " encoded.", 2);
    utilities::rmFile(file_in);
    std::atomic_fetch_add(&DATA->state.can_accept, 1);
    pushChunkSend(ti);
    return 0;
}
vector<string> utilities::extract(const std::string text, const std::string from, int count) {
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

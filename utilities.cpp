#define _FILE_OFFSET_BITS 64
#include "include_list.h"
#include "handle_IO.h"
#include "commands.h"
#include "network_helper.h"
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

string utilities::getExtension(const std::string &str) {
    size_t pos = str.rfind('.');
    std::string extension("");
    if (pos != std::string::npos)
        extension = str.substr(pos + 1);
    return extension;
}

string utilities::getBasename(const std::string &str) {
    size_t pos = str.rfind('/');
    std::string basename("");
    if (pos == std::string::npos)
        return std::string("");
    basename = str.substr(pos + 1);
    pos = basename.rfind('.');
    if (pos == std::string::npos)
        return basename;
    basename = basename.substr(0, pos);
    return basename;
}

long utilities::getFileSize(const std::string &file) {
    struct stat64 finfo;
    if (lstat64(file.c_str(), &finfo) == -1)
        return (-1);
    return finfo.st_size / 1000;
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


int utilities::checkFile(string &path) {
    struct stat info;

    if (stat (path.c_str(), &info) == -1){
        reportError("An error occured while controlling the file: " + path);
        return (-1);
    }
    std::string extension = getExtension(path);
    if (extension.empty()) {
        reportError("Invalid file extension.");
        return (-1);
    }
    vector<string> video_ext {"avi", "mkv", "ogg"};
    if (find(video_ext.begin(), video_ext.end(), extension) == video_ext.end()) {
        reportError("Unknown file extension");
        return (-1);
    }
    return (0);
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

int utilities::rmrDir(const char *dir, bool recursive) {
  DIR *d, *t;
  struct dirent *entry;
  char abs_fn[256];
  d = opendir(dir);
  if (d == NULL)
    return (-1);
  while ((entry = readdir(d))) {
    if ((strcmp(entry->d_name, ".") == 0) ||
      (strcmp(entry->d_name, "..") == 0))
      continue;
    sprintf(abs_fn, "%s/%s", dir, entry->d_name);
    if ((t = opendir(abs_fn))) {
      closedir(t);
      if(recursive)
        rmrDir(abs_fn, true);
      else
         return (-1);
    } else {
      if (unlink(abs_fn) == -1)
          return (-1);
    }
  }
  closedir(d);
  rmdir(dir);
  return (0);
}


int utilities::prepareDir(string &location, bool destroy) {
    if (mkdir(location.c_str(), 0700) == -1) {
        switch (errno) {
        case EEXIST:
            if (destroy) {
                if (rmrDir(location.c_str(), false) == -1)
                    return (-1);
                if (mkdir(location.c_str(), 0700) == -1)
                    return (-1);
            }
            break;
        default:
            return (-1);
            break;
        }
    }
    return (0);
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
    reportDebug("Encoding: " + file_in + " -> ", 2);
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
        DATA->state.can_accept++;
        return -1;
    }
    DATA->state.can_accept++;
    reportDebug("Chunk " + ti->name + " encoded.", 2);
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

string utilities::getTimestamp() {
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

int utilities::runExternal(string &stdo, std::string &stde, char *cmd, int numargs, ...) {
    pid_t pid;
    int pd_o[2], pd_e[2], j;
    size_t bufsize = 65536;
    char buf_o[bufsize], buf_e[bufsize];
    char *bo = buf_o, *be = buf_e;
    pipe(pd_o);
    pipe(pd_e);
    switch (pid = fork()) {
    case 0: {
        va_list arg_ptr;
        va_start (arg_ptr, numargs);
        char *args[numargs + 3];
        // TODO: safety!
        for(j = 0 ; j < numargs; ++j) {
            char *arg = va_arg(arg_ptr, char *);
            args[j] = arg;
        }
        args[j] = nullptr;
        va_end(arg_ptr);
        close(STDOUT_FILENO);
        close(pd_o[0]);
        dup(pd_o[1]);
        close(STDERR_FILENO);
        close(pd_e[0]);
        dup(pd_e[1]);
        char command[BUF_LENGTH];
        strcpy(command, cmd);
        if ((execvp(command, args)) == -1) {
            reportError("Error while spawning external command.");
            return (-1);
        }
        break;
        }
    case -1:
        close(pd_o[0]);
        close(pd_o[1]);
        close(pd_e[0]);
        close(pd_e[1]);
        return (-1);
        break;
    default:
        close(pd_o[1]);
        close(pd_e[1]);
        stdo = stde = "";
        int st;
        wait(&st);
        while(read(pd_o[0], bo, 1) == 1){
            bo++;
            //always positive anyway
            if((unsigned)(bo - buf_o) > bufsize){
                *bo = '\0';
                stdo += std::string(buf_o);
                bo = buf_o;
            }
        }
        while(read(pd_e[0], be, 1) == 1){
            be++;
            if((unsigned)(be - buf_e) > bufsize){
                *be = '\0';
                stde += std::string(buf_e);
                be = buf_e;
            }
        }
        stdo += std::string(buf_o);
        stde += std::string(buf_e);
        close(pd_o[0]);
        close(pd_e[0]);
        break;
    }
    return (0);
}

vector<string> utilities::split(const std::string &content, char sep) {
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

bool utilities::knownCodec(const std::string &cod) {
    vector<string> know = Data::getKnownCodecs();
    for (string &c : know) {
        if (c == cod)
            return (true);
    }
    return (false);
}

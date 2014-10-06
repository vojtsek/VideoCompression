#define _FILE_OFFSET_BITS 64
#include "common.h"
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

//int common::readCmd(stringstream &ins, cmd_storage_t &cmds, VideoState &state) {
//    string line, cmd_name;
//    getline(ins, line);
//    stringstream ss(line);
//    ss >> cmd_name;
//    if (cmd_name == "q")
//            return (1);
//    common::cursToInfo();
//    common::clearNlines(INFO_LINES);
//    try {
//        cmds.at(cmd_name)->execute(ss, state);
//    } catch (out_of_range) {
//        cmds["default"]->execute(ss, state);
//    }
//    return (0);
//}


int common::acceptCmd(cmd_storage_t &cmds) {
    wchar_t c;
    std::unique_lock<std::mutex> lck(DATA->IO_mtx, std::defer_lock);
    do {
        lck.lock();
        while (DATA->using_IO)
            DATA->cond.wait(lck);
        DATA->using_IO = true;
        c = getch();
        DATA->using_IO = false;
        lck.unlock();
        DATA->cond.notify_one();
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

struct sockaddr_storage common::addr2storage(const char *addrstr, int port, int family) {
    struct sockaddr_storage addr;
    if (family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) &addr;
        addr4->sin_family = family;
        addr4->sin_port = htons(port);
        inet_pton(family, addrstr, &(addr4->sin_addr));
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &addr;
        addr6->sin6_family = family;
        addr6->sin6_port = htons(port);
        inet_pton(family, addrstr, &(addr6->sin6_addr));
    }
    return addr;
}

string common::loadInput(const string &histf, const string &msg, bool save) {
    std::unique_lock<std::mutex> lck(DATA->IO_mtx, std::defer_lock);

    lck.lock();
    while (DATA->using_IO)
        DATA->cond.wait(lck);
    cursToQuestion();
    printw("%s", msg.c_str());
    DATA->using_IO = true;
    char line[LINE_LENGTH];
    curs_set(1);
    cursToCmd();
    printw(">");
    refresh();
    common::getLine(line, LINE_LENGTH, histf, save);
    curs_set(0);
    cursorToX(0);
    clrtoeol();
    cursToQuestion();
    clrtoeol();
    refresh();
    DATA->using_IO = false;
    lck.unlock();
    DATA->cond.notify_one();
    return string(line);
}

void common::listCmds() {
    printw("List of available commands:\n");
    show("help", "[command_name]");
    show("show", "parametres|jobs|neighbors");
    show("load", "/path/to/file");
    show("set", "[parameter=new_value[,...]]");
    show("start", "");
    show("quit", "");
}

string common::getExtension(const string &str) {
    size_t pos = str.rfind('.');
    string extension("");
    if (pos != string::npos)
        extension = str.substr(pos + 1);
    return extension;
}

string common::getBasename(const string &str) {
    size_t pos = str.rfind('/');
    string basename("");
    if (pos == string::npos)
        return string("");
    basename = str.substr(pos + 1);
    pos = basename.rfind('.');
    if (pos == string::npos)
        return basename;
    basename = basename.substr(0, pos);
    return basename;
}

long common::getFileSize(const string &file) {
    struct stat64 finfo;
    if (lstat64(file.c_str(), &finfo) == -1)
        return (-1);
    return finfo.st_size / 1000;
}

void common::clearNlines(int n) {
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

void common::reportFileProgress(const string &file, long desired) {
    long fs = 0, old = 0;
    int tries = 10;
    double percent;
    while (fs < desired) {
        fs = common::getFileSize(file);
        if (fs < 0)
            fs = 0;
        if (fs == old) {
            if (fs > 0)
                --tries;
            usleep(150000);
        } else
            tries = 10;
        if (!tries)
            break;
        old = fs;
        percent = (double) fs / (double) desired;
        common::printProgress(percent);
        usleep(10000);
    }
}

int common::checkFile(string &path) {
    struct stat info;

    if (stat (path.c_str(), &info) == -1){
        common::reportError("An error occured while controlling the file: " + path);
        return (-1);
    }
    string extension = getExtension(path);
    if (extension.empty()) {
        common::reportError("Invalid file extension.");
        return (-1);
    }
    vector<string> video_ext {"avi", "mkv", "ogg"};
    if (find(video_ext.begin(), video_ext.end(), extension) == video_ext.end()) {
        common::reportError("Unknown file extension");
        return (-1);
    }
    return (0);
}

int common::rmrDir(const char *dir, bool recursive) {
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


int common::prepareDir(string &location) {
    if (mkdir(location.c_str(), 0700) == -1) {
        switch (errno) {
        case EEXIST:
            if (rmrDir(location.c_str(), false) == -1)
                return (-1);
            if (mkdir(location.c_str(), 0700) == -1)
                return (-1);
            break;
        default:
            return (-1);
            break;
        }
    }
    return (0);
}

int common::encodeChunk(string &path, string &codec, string &extension) {
    string out, err;
    char cmd[BUF_LENGTH];
    string file_out = common::getBasename(path) + "." + extension;
    Measured<>::exec_measure(runExternal, out, err, cmd, 8, cmd,
             "-i", path.c_str(),
             "-vcodec", codec.c_str(),
             "-acodec", "copy",
             file_out.c_str());
    return (0);
}

vector<string> common::extract(const string text, const string from, int count) {
    vector<string> result;
    string word;
    stringstream ss(text);
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

string common::getTimestamp() {
    char stamp[16];
    sprintf(stamp, "%d", (int) time(NULL));
    return string(stamp);
}

void common::reportError(const string &err) {
    DATA->status_handler.add(err, ERROR);
    DATA->status_handler.print();
}

void common::reportSuccess(const string &msg) {
    DATA->status_handler.add(msg, SUCCESS);
    DATA->status_handler.print();
}

void common::reportStatus(const string &msg) {
    DATA->status_handler.add(msg, PLAIN);
    DATA->status_handler.print();
}

void common::reportDebug(const string &msg, int lvl) {
    if (lvl <= DEBUG_LEVEL) {
        DATA->status_handler.add(msg, DEBUG);
        DATA->status_handler.print();
    }
}

void common::initCurses() {
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    halfdelay(3);
    start_color();
    init_color(COLOR_GREY, 350, 350, 350);
    init_color(COLOR_CYAN, 500, 500, 1000);
    init_pair(RED, COLOR_RED, BG_COL);
    init_pair(GREEN, COLOR_GREEN, BG_COL);
    init_pair(BLUE, COLOR_BLUE, BG_COL);
    init_pair(YELLOWALL, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(CYANALL, COLOR_CYAN, COLOR_CYAN);
    init_pair(GREYALL, COLOR_GREY, COLOR_GREY);
    init_pair(BG, COLOR_WHITE, BG_COL);
    init_pair(INVERTED, BG_COL, COLOR_WHITE);
    wbkgd(stdscr, COLOR_PAIR(BG));
}

void common::printProgress(double percent) {
    common::cursToPerc();
    clrtoeol();
    printw("(%d%%)", (int) (percent * 100));
    attron(COLOR_PAIR(CYANALL));
    for(int i = 0; i < percent * (getmaxx(stdscr) - 7); ++i)
        printw("#");
    attroff(COLOR_PAIR(CYANALL));
    refresh();
}

void common::cursToCmd() {
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);
    move(max_y - 1, 0);
    clrtoeol();
}

void common::cursToInfo() {
    move(3, 0);
}

//TODO: still causes occasional problems
void common::cursToStatus() {
    if (!DATA->status_y) {
        DATA->status_y = getmaxy(stdscr) - 4;
    }
    static int status_y = DATA->status_y;
    move(status_y, 0);
}

void common::cursToQuestion() {
    if (!DATA->question_y)
        DATA->question_y = getmaxy(stdscr) - 2;
    move(DATA->question_y, 0);
}

void common::cursToPerc() {
    if (!DATA->perc_y)
        DATA->perc_y = getmaxy(stdscr) - 3;
    move(DATA->perc_y, 0);
}

void common::clearProgress() {
    common::cursToPerc();
    clrtoeol();
}

void common::cursorToX(int nx) {
    int y, x;
    getyx(stdscr, y, x);
    move(y, nx);
}

int common::getLine(char *line, int len, const string &histf, bool save) {
    HistoryStorage hist(histf);
    char *start = line;
    *line = '\0';

    nocbreak();
    cbreak();
    wchar_t c;
    int read = 0;
    while(++read <= len) {
        c = getch();
        if (c == KEY_UP) {
            common::cursorToX(1);
            clrtoeol();
            hist.prev();
            try {
                printw(hist.getCurrent().c_str());
                read = hist.getCurrent().size();
                strncpy(start, hist.getCurrent().c_str(), read);
                line = start + read;
            } catch (...) {}
        } else if(c == KEY_DOWN) {
            common::cursorToX(1);
            clrtoeol();
            hist.next();
            try {
                printw(hist.getCurrent().c_str());
                read = hist.getCurrent().size();
                strncpy(start, hist.getCurrent().c_str(), read);
                line = start + read;
            } catch (...) {}
        } else if (c == 8) {
            int y, x;
            getyx(stdscr, y, x);
            --x;
            if (x < 1)
                ++x;
            else {
                --read;
                --line;
            }
            move(y, x);
            delch();
        } else if (common::isAcceptable(c)) {
            printw("%c", c);
            *(line++) = c;
        }
        refresh();
        if ((c == '\n'))
            break;
    }
    *line = '\0';
    if (*start != '\0')
        hist.save(string(start));
    if (save)
        hist.write();
    nocbreak();
    halfdelay(3);
    if (read <= len)
        return (0);
    else
//        buffer too short
        return (-1);
}

bool common::isAcceptable(char c) {
    vector<char> acc = {'/', '.', '_', ' ', '=', '-'};
    if ((!isgraph(c)) || (iscntrl(c)))
        return false;
    return ((isalnum(c)) || (find(acc.begin(), acc.end(), c) != acc.end()));
}

int common::runExternal(string &stdo, string &stde, char *cmd, int numargs, ...) {
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
            common::reportError("Error while spawning external command.");
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
                stdo += string(buf_o);
                bo = buf_o;
            }
        }
        while(read(pd_e[0], be, 1) == 1){
            be++;
            if((unsigned)(be - buf_e) > bufsize){
                *be = '\0';
                stde += string(buf_e);
                be = buf_e;
            }
        }
        stdo += string(buf_o);
        stde += string(buf_e);
        close(pd_o[0]);
        close(pd_e[0]);
        break;
    }
    return (0);
}

void common::reportTime(const string &file, double time) {
    string fn("results/" + common::getBasename(file) + ".measured");
    ofstream out;
    out.open(fn, ofstream::app);
    stringstream msg;
    out << time << endl;
    out.close();
    msg << "The operation took " << time << " seconds.";
    reportStatus(msg.str());
}

vector<string> common::split(const string &content, char sep) {
    size_t pos;
    vector<string> result;
    string remaining(content);
    while((pos = remaining.find(sep)) != string::npos) {
        result.push_back(remaining.substr(0, pos));
        remaining = remaining.substr(pos + 1, remaining.length());
    }
    result.push_back(remaining);
    return result;
}

bool common::knownCodec(const string &cod) {
    vector<string> know = Data::getKnownCodecs();
    for (string &c : know) {
        if (c == cod)
            return (true);
    }
    return (false);
}

bool common::cmpStorages(struct sockaddr_storage &s1, struct sockaddr_storage &s2) {
    if (((struct sockaddr *) &s1)->sa_family != ((struct sockaddr *) &s2)->sa_family)
        return false;
    if (((struct sockaddr *) &s1)->sa_family == AF_INET) {
        if ((((struct sockaddr_in *) &s1)->sin_port == ((struct sockaddr_in *) &s2)->sin_port))
                //(((struct sockaddr_in *) &s1)->sin_addr == ((struct sockaddr_in *) &s2)->sin_addr))
            return true;
        else
            return false;
    }
    //TODO: maybe bad
    if (((struct sockaddr *) &s1)->sa_family == AF_INET6) {
        if ((((struct sockaddr_in6 *) &s1)->sin6_port == ((struct sockaddr_in6 *) &s2)->sin6_port))
               // (((struct sockaddr_in6 *) &s1)->sin6_addr == ((struct sockaddr_in6 *) &s2)->sin6_addr))
            return true;
        else
            return false;
    }
    return false;
}

bool common::addrIn(struct sockaddr_storage &st, std::vector<NeighborInfo> list) {

    for (NeighborInfo &n : list) {
        if (cmpStorages(n.address, st))
            return true;
    }
    return false;
}

//TODO getsockname doesnt fail if short structure was supplied!
struct sockaddr_storage common::getHostAddr(int fd) {
    struct sockaddr_storage in4, in6;
    struct sockaddr_in *in4p = (struct sockaddr_in *) &in4;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &in6;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
    if (DATA->IPv4_ONLY) {
        in4p->sin_family = AF_INET;
        if (getsockname(fd, (struct sockaddr *) in4p, &size4) == -1) {
            reportDebug("Not an IPv4 addr.", 4);
        } else
            return (in4);
    } else {
        in6p->sin6_family = AF_INET6;
        if (getsockname(fd, (struct sockaddr *) in6p, &size6) == -1) {
            reportDebug("Not an IPv6 addr.", 4);
        } else
            return (in6 );
    }
    reportDebug("Failed to get host's address.", 2);
    return (in4);
}

struct sockaddr_storage common::getPeerAddr(int fd) {
    struct sockaddr_storage in4, in6;
    struct sockaddr_in *in4p = (struct sockaddr_in *) &in4;
    struct sockaddr_in6 *in6p = (struct sockaddr_in6 *) &in6;
    bzero(&in4p->sin_addr, INET_ADDRSTRLEN);
    bzero(&in6p->sin6_addr, INET6_ADDRSTRLEN);
    socklen_t size4 = sizeof (*in4p), size6 = sizeof(*in6p);
    if (getpeername(fd, (struct sockaddr *) in4p, &size4) == -1) {
        reportDebug("Not an IPv4 addr.", 4);
    } else
        return (in4);
    if (getpeername(fd, (struct sockaddr *) in6p, &size6) == -1) {
        reportDebug("Not an IPv6 addr.", 4);
    } else
        return (in4);
    reportDebug("Failed to get host's address.", 2);
    return (in4);
}

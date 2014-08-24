#include "common.h"
#include "commands.h"
#include "defines.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>


using namespace std;

int common::readCmd(istream &ins, map<string, Command *> &cmds, State &state) {
    string line, cmd_name;
    getline(ins, line);
    stringstream ss(line);
    ss >> cmd_name;
    if (cmd_name == "q")
            return (1);
    try {
        cmds.at(cmd_name)->execute(ss, state);
    } catch (out_of_range) {
        cmds["default"]->execute(ss, state);
    }

    return (0);
}

void common::listCmds() {
    cout << "List of available commands:" << endl;
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
        cerr << red << "Unknown file extension." << defaultFg << endl;
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

void common::printInfo(const string &msg) {
    cout << msg << endl;
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
    cerr << red << err << defaultFg << endl;
}

int common::runExternal(string &stdo, string &stde, const string &cmd, int numargs, ...) {
    pid_t pid;
    int pd_o[2], pd_e[2];
    size_t bufsize = 65536;
    char buf_o[bufsize], buf_e[bufsize];
    char *bo = buf_o, *be = buf_e;
    pipe(pd_o);
    pipe(pd_e);
    switch (pid = fork()) {
    case 0: {
        va_list arg_ptr;
        va_start (arg_ptr, numargs);
        char *args[numargs + 1];
        int i;
        // TODO: safety!
        for(i = 0; i < numargs; ++i) {
            char *arg = va_arg(arg_ptr, char *);
            args[i] = arg;
        }
        args[i] = nullptr;
        va_end(arg_ptr);
        close(STDOUT_FILENO);
        close(pd_o[0]);
        dup(pd_o[1]);
        close(STDERR_FILENO);
        close(pd_e[0]);
        dup(pd_e[1]);
        if ((execvp(cmd.c_str(), args)) == -1) {
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
            if(bo - buf_o > bufsize){
                stdo += string(buf_o);
                bo = buf_o;
            }
        }
        while(read(pd_e[0], be, 1) == 1){
            be++;
            if(be - buf_e > bufsize){
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

ostream &common::red(ostream &out){
    out << CSI << "01;31m";
    return out;
}

ostream &common::black(ostream &out){
    out << CSI << "00;30m";
    return out;
}

ostream &common::green(ostream &out){
    out << CSI << "01;32m";
    return out;
}

ostream &common::white(ostream &out){
    out << CSI << "01;37m";
    return out;
}

ostream &common::gray(ostream &out){
    out << CSI << "01;30m";
    return out;
}


ostream &common::defaultFg(ostream &out){
    out << CSI << "00;39m";
    return out;
}

ostream &common::grayBg(ostream &out){
    out << CSI << "01;40m";
    return out;
}

ostream &common::defaultBg(ostream &out){
    out << CSI << "00;49m";
    return out;
}

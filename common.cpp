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
#include <sys/wait.h>
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

int common::checkFile(string &path) {
    struct stat info;

    if (stat (path.c_str(), &info) == -1){
        common::reportError("An error occured while controlling the file: " + path);
        return (-1);
    }
    size_t pos = path.rfind('.');
    string extension = path.substr(pos + 1);
    vector<string> video_ext {"avi", "mkv", "ogg"};
    if (find(video_ext.begin(), video_ext.end(), extension) == video_ext.end()) {
        cerr << red << "Unknown filename extension." << defaultFg << endl;
        return (-1);
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

ostream &common::defaultFg(ostream &out){
    out << CSI << "00;39m";
    return out;
}

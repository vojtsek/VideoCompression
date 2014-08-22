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
    cout << setw(10) << "help" <<  setw(35) << "[command_name]" << endl;
    cout << setw(10) << "show" <<  setw(35) << "parametres|jobs|neighbors" << endl;
    cout << setw(10) << "load" <<  setw(35) << "/path/to/file" << endl;
    cout << setw(10) << "set" <<  setw(35) << "parameter=new_value[,...]" << endl;
    cout << setw(10) << "start" <<  setw(35) << endl;
    cout << setw(10) << "quit" <<  setw(35) << endl;
}

int common::checkFile(string &path) {
    struct stat info;

    if (stat (path.c_str(), &info) == -1){
        cerr << red << "An error occured while controlling the file: " << path << defaultFg << endl;
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

vector<string> common::extract(string &text, string &from, int count) {
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
            if (!--count)
                break;
        }
    }

    return result;
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

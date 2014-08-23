#include "commands.h"
#include "common.h"
#include "defines.h"

#include <iostream>
#include <sstream>
#include <string>
#include <map>

using namespace std;
using namespace common;

void Command::execute(stringstream &, State &) {
        common::listCmds();
}

void CmdHelp::execute(stringstream &ss, State &state) {
    string word;
    while (ss.good()){
        ss >> word;
        cout << red << word << defaultFg << endl;
    }
}

void CmdShow::execute(stringstream &ss, State &) {
    cout << "List yourself" << endl;
}

void CmdStart::execute(stringstream &, State &) {
    cout << "Help yourself" << endl;
}

void CmdSet::execute(stringstream &ss, State &) {

}

void CmdLoad::execute(stringstream &ss, State &state){
    string path, out, err, bitrate;
    vector<string> res;
    if  (ss.good()) {
        ss >> path;
    }
    if (path.empty()) {
        cerr << red << "File path not provided." << defaultFg << endl;
        path = "/home/vojcek/futu.avi";
//        return;
    }
    if (checkFile(path) == -1){
        cerr << red << "Loading the file " << path << " failed." << defaultFg << endl;
        return;
    }
    state.fpath = path;
    cout << green << path << " loaded." << defaultFg << endl;
    runExternal(out, err, "ffprobe", 5, "ffprobe", "/home/vojcek/futu.avi", "-show_format", "-print_format", "json");
    bitrate = extract(out, "\"bit_rate\":", 2)[1];
    cout << bitrate << endl;
}

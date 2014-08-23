#include "commands.h"
#include "common.h"
#include "defines.h"

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <stdio.h>

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

void CmdShow::execute(stringstream &ss, State &state) {
    string what;
    ss >> what;
    if (what == "jobs") {

    } else if (what == "neighbors"){

    } else {
        state.printState();
    }
}

void CmdStart::execute(stringstream &, State &state) {
    string out, err;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[32], output[32], current[32];
    printInfo("Starting");
    state.printState();
    mins = state.secs_per_chunk / 60;
    secs = state.secs_per_chunk % 60;
    sprintf(chunk_duration, "00:%02d:%02d", mins, secs);
    for (int i = 0; i < state.c_chunks; ++i) {
        elapsed = i * state.secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        sprintf(current, "%02d:%02d:%02d", hours, mins, secs);
        sprintf(output, "output/%d_splitted.avi", i);
        runExternal(out, err, "ffmpeg", 8, "ffmpeg",
                    "-i", state.fpath.c_str(),
                    "-ss", current,
                    "-t", chunk_duration,
                    output);
        printf("Run %s %s %s %s %s %s %s %s\n", "ffmpeg",
                    "-i", state.fpath.c_str(),
                    "-ss", current,
                    "-t", chunk_duration,
                    output);
    }
}

void CmdSet::execute(stringstream &ss, State &) {

}

void CmdLoad::execute(stringstream &ss, State &state){
    string path, out, err, help;
    double duration, bitrate, fsize;
    vector<string> res;
    if (ss.good()) {
        ss >> path;
    }
    path = "/data/futu.avi";
    if (path.empty()) {
        reportError("File path not provided.");
        return;
    }
    if (checkFile(path) == -1){
        reportError("Loading the file " + path + " failed");
        return;
    }
    state.fpath = path;
    cout << green << path << " loaded." << defaultFg << endl;
    runExternal(out, err, "ffprobe", 5, "ffprobe", state.fpath.c_str(), "-show_format", "-print_format", "json");
    if (err.find("Invalid data") != string::npos) {
        reportError("Invalid video file");
        state.resetFileInfo();
        return;
    }
    res = extract(out, "\"bit_rate\":", 2);
    try {
        help = res.at(1);
        stringstream ssd(help.substr(1, help.length()));
        ssd >> bitrate;
        state.bitrate = bitrate / 8;
    } catch (...) {
        reportError("Error while getting information about file.");
        state.resetFileInfo();
        return;
    }
    res = extract(out, "\"duration\":", 2);
    try {
        help = res.at(1);
        stringstream ssd(help.substr(1, help.length()));
        ssd >> duration;
        state.duration = duration;
    } catch (...) {
        reportError("Error while getting information about file.");
        state.resetFileInfo();
        return;
    }
    res = extract(out, "\"size\":", 2);
    try {
        help = res.at(1);
        stringstream ssd(help.substr(1, help.length()));
        ssd >> fsize;
        state.fsize = fsize;
    } catch (...) {
        reportError("Error while getting information about file.");
        state.resetFileInfo();
        return;
    }
    state.secs_per_chunk = CHUNK_SIZE  / (bitrate / 8);
}

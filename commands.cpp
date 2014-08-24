#include "commands.h"
#include "common.h"
#include "defines.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <stdio.h>

using namespace std;
using namespace common;

int splitVideo(State &state) {
    string out, err;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[32], output[32], current[32], msg[256];

    if (prepareDir(state.dir_location) == -1) {
        reportError("Failed to create the job directory.");
        return (-1);
    }
    mins = state.secs_per_chunk / 60;
    secs = state.secs_per_chunk % 60;
    sprintf(chunk_duration, "00:%02d:%02d", mins, secs);
    cout << "Beginning to split " << state.fpath << endl;
    for (int i = 0; i < state.c_chunks; ++i) {
        elapsed = i * state.secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        sprintf(current, "%02d:%02d:%02d", hours, mins, secs);
        sprintf(output, "%s/%03d_splitted.avi", state.dir_location.c_str(), i);
        if (runExternal(out, err, "ffmpeg", 8, "ffmpeg",
                    "-i", state.fpath.c_str(),
                    "-ss", current,
                    "-t", chunk_duration,
                    output) == -1)
            return (-1);
        if (err.find("Conversion failed") != string::npos) {
            sprintf(msg, "%s %s %s %s %s %s %s %s\n", "ffmpeg",
                "-i", state.fpath.c_str(),
                "-ss", current,
                "-t", chunk_duration,
                output);
            reportError(msg);
            return (-1);
        }
        double percent = (double) (i+1) / state.c_chunks;
        string pretty;
        cout << setw(74) << left;
        for(int i = 0; i < percent * 74; ++i)
            pretty.push_back('#');
        cout << pretty;
        cout << "(" << (int) (percent * 100) << "%)";
        cout << endl;
    }
    cout << green << state.fpath << " successfuly splitted." << defaultFg << endl;
    return (0);
}

int joinVideo(State &state) {
    string out, err, list_loc(state.dir_location + "/join_list.txt"), output(state.basename + "_output." + state.extension);
    ofstream ofs(list_loc);
    char file[128];

    cout << "Beginning to join the file." << endl;
    for (unsigned int i = 0; i < state.c_chunks; ++i) {
        sprintf(file, "file '%03d_splitted.avi'", i);
        ofs << file << endl;
    }
    ofs.flush();
    ofs.close();
    if (runExternal(out, err, "ffmpeg", 8, "ffmpeg",
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    output.c_str()) == -1)
        return (-1);
    if (err.find("failed") != string::npos)
        return (-1);
    cout << green << "Successfuly joined, result: " << output << defaultFg << endl;
    return (0);
}

void Command::execute(stringstream &, State &) {
        common::listCmds();
}

void CmdHelp::execute(stringstream &ss, State &) {
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
    state.printState();
    if (splitVideo(state) == -1) {
        reportError("Error while splitting the video file.");
        return;
    }
    if (joinVideo(state) == -1) {
        reportError("Error while joining the video file.");
        return;
    }
    rmrDir(state.dir_location.c_str(), false);
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
    state.extension = getExtension(path);
    state.basename = getBasename(path);
    cout << green << path << " loaded." << defaultFg << endl;
    if (runExternal(out, err, "ffprobe", 5, "ffprobe", state.fpath.c_str(), "-show_format", "-print_format", "json") == -1) {
        reportError("Error while getting video information.");
        return;
    }
    if (err.find("Invalid data") != string::npos) {
        reportError("Invalid video file");
        state.resetFileInfo();
        return;
    }
    char dir_name[32];
    sprintf(dir_name, "job_%s", getTimestamp().c_str());
    state.dir_location = string(dir_name);
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

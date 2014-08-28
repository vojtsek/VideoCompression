#include "commands.h"
#include "common.h"
#include "defines.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <stdio.h>

using namespace std;
using namespace common;
using namespace rapidjson;

int splitVideo(State &state) {
    string out, err;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH], current[BUF_LENGTH], msg[BUF_LENGTH];

    if (prepareDir(state.dir_location) == -1) {
        reportError("Failed to create the job directory.");
        return (-1);
    }
    mins = state.secs_per_chunk / 60;
    secs = state.secs_per_chunk % 60;
    snprintf(chunk_duration, BUF_LENGTH, "00:%02d:%02d", mins, secs);
    printw("Beginning to split %s", state.finfo.fpath.c_str());
    for (unsigned int i = 0; i < state.c_chunks; ++i) {
        double percent = (double) i / state.c_chunks;
        printProgress(percent);
        elapsed = i * state.secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        snprintf(current, BUF_LENGTH, "%02d:%02d:%02d", hours, mins, secs);
        snprintf(output, BUF_LENGTH, "%s/%03d_splitted.avi", state.dir_location.c_str(), i);
        if (runExternal(out, err, "ffmpeg", 8, "ffmpeg",
                    "-i", state.finfo.fpath.c_str(),
                    "-ss", current,
                    "-t", chunk_duration,
                    output) == -1)
            return (-1);
        if (err.find("Conversion failed") != string::npos) {
            sprintf(msg, "%s %s %s %s %s %s %s %s\n", "ffmpeg",
                "-i", state.finfo.fpath.c_str(),
                "-ss", current,
                "-t", chunk_duration,
                output);
            reportError(msg);
            return (-1);
        }
    }
    printProgress(1);
    reportSuccess("\nFile successfuly splitted.\n");
    return (0);
}

int joinVideo(State &state) {
    string out, err, list_loc(state.dir_location + "/join_list.txt"), output(state.finfo.basename + "_output." + state.finfo.extension);
    ofstream ofs(list_loc);
    char file[BUF_LENGTH];

    printw("Beginning to join the file.\n");
    for (unsigned int i = 0; i < state.c_chunks; ++i) {
        snprintf(file, BUF_LENGTH, "file '%03d_splitted.avi'", i);
        try {
            ofs << file << endl;
        } catch (...) {} // TODO: handle exception
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
    reportSuccess("Succesfully joined.\n");
    return (0);
}

void Command::execute(stringstream &, State &) {
        common::listCmds();
}

void CmdHelp::execute(stringstream &ss, State &) {
    string word;
    while (ss.good()){
        ss >> word;
    }
}

void CmdShow::execute(stringstream &ss, State &state) {
    string what;
    ss >> what;
    if (what == "jobs") {

    } else if (what == "neighbors"){

    } else {
        if (state.finfo.fpath.empty()) {
            reportError("Please load the file first.");
            return;
        }
        state.printState();
    }
}

void CmdStart::execute(stringstream &, State &state) {
    if (state.finfo.fpath.empty()) {
        reportError("Please load the file first.");
        return;
    }
    if (checkFile(state.finfo.fpath) == -1) {
        reportError("Invalid file.");
        return;
    }
    state.printState();
    printw("\nSplitting the file %s:\n", state.finfo.fpath.c_str());
    refresh();
    if (splitVideo(state) == -1) {
        reportError("Error while splitting the video file.");
        rmrDir(state.dir_location.c_str(), false);
        return;
    }
    if (joinVideo(state) == -1) {
        reportError("Error while joining the video file.");
        rmrDir(state.dir_location.c_str(), false);
        return;
    }
    rmrDir(state.dir_location.c_str(), false);
}

void CmdSet::execute(stringstream &ss, State &state) {
    string line;
    vector<string> params, current;
    while (ss.good()) {
        getline(ss, line);
        params = split(line, ',');
        for (string a : params) {
            current = split(a, '=');
            for (string &b : current)
                printw("%s\n", b.c_str());
        }
    }
}

void CmdLoad::execute(stringstream &ss, State &state){
    string path, out, err, help, err_msg("Error loading the file");
    Document document;
    stringstream ssd;
    finfo_t finfo;

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
    finfo.fpath = path;
    finfo.extension = getExtension(path);
    finfo.basename = getBasename(path);
    if (runExternal(out, err, "ffprobe", 6, "ffprobe", finfo.fpath.c_str(), "-show_streams", "-show_format", "-print_format", "json") == -1) {
        reportError("Error while getting video information.");
        return;
    }
    if (err.find("Invalid data") != string::npos) {
        reportError("Invalid video file");
        state.resetFileInfo();
        return;
    }
    if(document.Parse(out.c_str()).HasParseError()) {
        reportError(err_msg);
        return;
    }
    if (!document.HasMember("format")) {
        reportError(err_msg);
        return;
    }
    if(!document["format"].HasMember("bit_rate")) {
        reportError(err_msg);
        return;
    }
    ssd.clear();
    ssd.str(document["format"]["bit_rate"].GetString());
    ssd >> finfo.bitrate;
    finfo.bitrate /= 8;

    if(!document["format"].HasMember("duration")) {
        reportError(err_msg);
        return;
    }
    ssd.clear();
    ssd.str(document["format"]["duration"].GetString());
    ssd >> finfo.duration;

    if(!document["format"].HasMember("size")) {
        reportError(err_msg);
        return;
    }
    ssd.clear();
    ssd.str(document["format"]["size"].GetString());
    ssd >> finfo.fsize;

    if(!document["format"].HasMember("format_name")) {
        reportError(err_msg);
        return;
    }
    finfo.format = document["format"]["format_name"].GetString();

    if(!document.HasMember("streams")) {
        reportError(err_msg);
        return;
    }
    const Value &streams = document["streams"];
    bool found = false;
    if (!streams.IsArray()) {
        reportError("Invalid video file");
        return;
    }
    for(SizeType i = 0; i < streams.Size(); ++i) {
        if (streams[i].HasMember("codec_type") && (streams[i]["codec_type"].GetString() == string("video"))) {
            finfo.codec = streams[i]["codec_name"].GetString();
            found = true;
            break;
        }
    }
    if (!found) {
        reportError("Invalid video file");
        return;
    }
    state.loadFileInfo(finfo);
    reportSuccess(finfo.fpath + " loaded.");
}

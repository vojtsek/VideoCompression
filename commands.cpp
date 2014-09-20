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
#include <vector>
#include <thread>
#include <algorithm>
#include <map>
#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace common;
using namespace rapidjson;

int splitVideo(State &state) {
    string out, err;
    double sum = 0.0;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH], current[BUF_LENGTH], msg[BUF_LENGTH], cmd[BUF_LENGTH];

    if (prepareDir(state.dir_location) == -1) {
        reportError("Failed to create the job directory.");
        return (-1);
    }
    mins = state.secs_per_chunk / 60;
    secs = state.secs_per_chunk % 60;
    snprintf(chunk_duration, BUF_LENGTH, "00:%02d:%02d", mins, secs);
    reportStatus("Splitting file: " + state.finfo.fpath);
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
        snprintf(cmd, BUF_LENGTH, "ffmpeg");
        cursToStatus();
        sum += Measured<>::exec_measure(runExternal, out, err, cmd, 12, cmd,
                    "-ss", current,
                    "-i", state.finfo.fpath.c_str(),
                    "-v", "quiet",
                    "-c", "copy",
                    "-t", chunk_duration,
                    output);
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
    reportSuccess("File successfuly splitted.");
    reportTime("/splitting.sp", sum / 1000);
    return (0);
}

int joinVideo(State &state) {
    string out, err, list_loc(state.dir_location + "/join_list.txt"), output(state.finfo.basename + "_output." + state.finfo.extension);
    ofstream ofs(list_loc);
    stringstream ss;
    char file[BUF_LENGTH], cmd[BUF_LENGTH], file_in[BUF_LENGTH], file_out[BUF_LENGTH];
    long sum_size = 0;
    double duration = 0;
    unlink(output.c_str());
    errno = 0;
    reportStatus("Joining the file: " + output);
    snprintf(cmd, BUF_LENGTH, "ffmpeg");
    for (unsigned int i = 0; i < state.c_chunks; ++i) {
        snprintf(file_in, BUF_LENGTH, "%s/%03d_splitted.avi", state.dir_location.c_str(), i);
        snprintf(file_out, BUF_LENGTH, "%s/%03d_splitted.mkv", state.dir_location.c_str(), i);
        Measured<>::exec_measure(runExternal, out, err, cmd, 8, cmd,
                                 "-i", file_in,
                                 "-vcodec", "libx264",
                                 "-acodec", "copy",
                                 file_out);
        snprintf(file, BUF_LENGTH, "file '%03d_splitted.mkv'", i);
        try {
            ss.clear();
            ss.str("");
            ss << state.dir_location << "/" << setfill('0') << setw(3) << i << "_splitted.mkv";
            sum_size += getFileSize(ss.str());
            ofs << file << endl;
        } catch (...) {} // TODO: handle exception
    }
    ofs.flush();
    ofs.close();
    thread thr(reportFileProgress, output, sum_size);
    duration = Measured<>::exec_measure(runExternal, out, err, cmd, 8, cmd,
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    output.c_str());
    if (err.find("failed") != string::npos) {
        thr.detach();
        return (-1);
    }
    thr.join();
    printProgress(1);
    reportSuccess("Succesfully joined.");
    reportTime("/joining.j", duration / 1000);
    return (0);
}

void Command::execute(State &) {
    common::cursToInfo();
}

void CmdHelp::execute(State &) {
}

void CmdShow::execute(State &state) {
    string what = loadInput("show.hist", "", true);
    if (what == "jobs") {

    } else if (what == "neighbors"){

    } else {
        if (state.finfo.fpath.empty()) {
            reportError("Please load the file first.");
            return;
        }
        cursToInfo();
        state.printState();
    }
}

void CmdStart::execute(State &state) {
    if (state.finfo.fpath.empty()) {
        reportError("Please load the file first.");
        return;
    }
    if (checkFile(state.finfo.fpath) == -1) {
        reportError("Invalid file.");
        return;
    }
    cursToInfo();
    state.printState();
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

void CmdSetCodec::execute(State &state) {
    string in = loadInput("codecs", "Enter new codec:", false);
    if (knownCodec(in)) {
        state.o_codec = in;
        reportSuccess("Output codec set to: " + in);
    } else {
        reportError("Unknown codec: " + in);
        stringstream msg;
        msg << "Available codecs: ";
        for (string c : Data::getKnownCodecs())
            msg << c << ", ";
        reportStatus(msg.str().substr(0, msg.str().length() - 2));
    }
}

void CmdSetChSize::execute(State &state) {
    string in = loadInput("", "Enter new chunk size (kB):", false);
    size_t nsize = CHUNK_SIZE;
    stringstream ss(in), msg;
    ss >> nsize;
    state.changeChunkSize(nsize * 1024);
    msg << "Chunk size set to: " << nsize;
    reportSuccess(msg.str());
}

void CmdSet::execute(State &state) {
    string line = loadInput("set.history", "What option set?", false);
    if (line.find("codec") != string::npos)
        Data::getInstance()->cmds[SET_CODEC]->execute(state);
    else if (line.find("chunksize") != string::npos)
        Data::getInstance()->cmds[SET_SIZE]->execute(state);
    else
        reportStatus("Available options: 'codec'', 'chunksize'");
}

void CmdLoad::execute(State &state){
    string path, out, err, err_msg("Error loading the file: ");
    Document document;
    stringstream ssd;
    finfo_t finfo;

    path = loadInput("paths.history", "Enter a file path:", true);
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
        reportError(err_msg + "Parse error");
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

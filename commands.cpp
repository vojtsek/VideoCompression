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
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace common;
using namespace rapidjson;

void Command::execute() {
    common::cursToInfo();
}

void CmdHelp::execute() {
}

void CmdShow::execute() {
    string what = loadInput("show.hist", "", true);
    if (what == "jobs") {

    } else if (what == "neighbors"){

    } else {
        if (state->finfo.fpath.empty()) {
            reportError("Please load the file first.");
            return;
        }
        cursToInfo();
        state->printVideoState();
    }
}

void CmdStart::execute() {
    if (state->finfo.fpath.empty()) {
        reportError("Please load the file first.");
        return;
    }
    if (checkFile(state->finfo.fpath) == -1) {
        reportError("Invalid file.");
        return;
    }
    cursToInfo();
    state->printVideoState();
    refresh();
    if (state->split() == -1) {
        reportError("Error while splitting the video file.");
        rmrDir(state->dir_location.c_str(), false);
        return;
    }
    if (state->join() == -1) {
        reportError("Error while joining the video file.");
        rmrDir(state->dir_location.c_str(), false);
        return;
    }
    rmrDir(state->dir_location.c_str(), false);
}

void CmdSetCodec::execute() {
    string in = loadInput("codecs", "Enter new codec:", false);
    if (knownCodec(in)) {
        state->o_codec = in;
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

void CmdSetChSize::execute() {
    string in = loadInput("", "Enter new chunk size (kB):", false);
    size_t nsize = CHUNK_SIZE;
    stringstream ss(in), msg;
    ss >> nsize;
    state->changeChunkSize(nsize * 1024);
    msg << "Chunk size set to: " << nsize;
    reportSuccess(msg.str());
}

void CmdSet::execute() {
    string line = loadInput("set.history", "What option set?", false);
    if (line.find("codec") != string::npos)
        Data::getInstance()->cmds[SET_CODEC]->execute();
    else if (line.find("chunksize") != string::npos)
        Data::getInstance()->cmds[SET_SIZE]->execute();
    else
        reportStatus("Available options: 'codec'', 'chunksize'");
}

void CmdLoad::execute(){
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
        state->resetFileInfo();
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
    state->loadFileInfo(finfo);
    reportSuccess(finfo.fpath + " loaded.");
}

int NetworkCommand::connectPeer(struct sockaddr_storage *addr) {
    int sock, family;
    char errormsg[BUF_LENGTH];
    family = ((struct sockaddr *) addr)->sa_family;
    if ((sock = socket(family, SOCK_STREAM, 6)) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportError("Failed to create socket for connection." + string(errormsg));
        return (-1);
    }
    if (connect(sock, (struct sockaddr *) addr, sizeof (*addr)) == -1) {
        strerror_r(errno, errormsg, BUF_LENGTH);
        errno = 0;
        reportError("Failed to connect to remote peer." + string(errormsg));
        return(-1);
    }
    reportSuccess("Connected");
    return (sock);
}

void NetworkCommand::execute() {}

void CmdAsk::execute() {
    struct sockaddr_storage addr;
    struct sockaddr_in *addr4 = (struct sockaddr_in *) &addr;
    reportStatus("Connecting...");
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(25000);
    char astr[] = "127.0.0.1";
    inet_pton(AF_INET, astr, &(addr4->sin_addr));
    handler->confirmNeighbor(addr);
    /* if ((sock = connectPeer((struct sockaddr_storage *) &addr)) == -1) {
        reportError("Failed to establish a connection");
        return;
    }
    handler->spawnConnection(OUTGOING, NULL, sock, RESPOND); */
}

void CmdRespond::execute() {
    reportStatus("Responding");
    CMDS action = REACT;
    sendCmd(fd, action);
    sendSth("Hi\0", fd, 3);
}

void CmdReact::execute() {
    char response[BUF_LENGTH];
    recvSth(response, fd, BUF_LENGTH);
    reportSuccess("Received: " + string(response));
    close(fd);
}

void CmdConfirmPeer::execute() {
    CMDS action = CONFIRM_HOST;
    bool ok = true;
    int count = handler->getNeighborCount();
    struct sockaddr_storage addr;
    sendCmd(fd, action);
    sendSth(ok, fd);
    sendSth(count, fd);
    addr = addr2storage("1.1.1.1", 0, AF_INET);
    sendSth(addr, fd);
    for (auto &neighbor : handler->getNeighbors()) {
        addr = neighbor.address;
        sendSth(addr, fd);
    }
}

void CmdConfirmHost::execute() {
    struct sockaddr_storage addr;
    bool ok;
    int count;
    recvSth(ok, fd);
    if (ok)
        reportSuccess("Neighbor confirmed.");
    if (!ok) {
        close(fd);
        return;
    }
    recvSth(count, fd);
    recvSth(addr, fd);
    handler->addNewNeighbor(false, addr);
    for (int i = 0; i < count; ++i) {
        recvSth(addr, fd);
        reportDebug("Received one address.");
        handler->addNewNeighbor(true, addr);
    }
    close(fd);
}

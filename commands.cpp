#include "commands.h"
#include "common.h"
#include "handle_IO.h"
#include "network_helper.h"
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
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace common;
using namespace rapidjson;

void Command::execute() {
    cursToInfo();
}

void CmdHelp::execute() {
}

void CmdDef::execute() {
    reportSuccess("iiii");
}

void CmdShow::execute() {
    string what = loadInput("show.hist", "", false);
    if (what == "jobs") {

    } else if (what.find( "neighbors") != string::npos) {
        cursToInfo();
        for(auto &n : handler->getNeighbors()) {
            n.printInfo();
        }
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
        DATA->cmds[SET_CODEC]->execute();
    else if (line.find("chunksize") != string::npos)
        DATA->cmds[SET_SIZE]->execute();
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
    family = ((struct sockaddr *) addr)->sa_family;
    if ((sock = socket(family, SOCK_STREAM, 6)) == -1) {
        reportDebug("Failed to create socket for connection." + string(strerror(errno)), 1);
        return (-1);
    }
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
            &so_linger, sizeof (so_linger)) == -1) {
        reportDebug("Failed to set option to listening socket." + string(strerror(errno)), 1);
        return (-1);
   }
    if (connect(sock, (struct sockaddr *) addr, sizeof (*addr)) == -1) {
        reportDebug("Failed to connect to remote peer." + string(strerror(errno)) + MyAddr(*addr).get(), 1);
        return(-1);
    }
    MyAddr mad(*addr);
    reportDebug("Connected to " + mad.get(), 5);
    return (sock);
}


bool CmdAsk::execute(int fd, struct sockaddr_storage &address, void *data) {
            wscrl(DATA->io_data.status_win, -3);
            wrefresh(DATA->io_data.status_win);
}

bool CmdRespond::execute(int fd, struct sockaddr_storage &address, void *data) {
    reportStatus("Responding");
    CMDS action = REACT;
    sendCmd(fd, action);
    sendSth("Hi\0", fd, 3);
}

bool CmdReact::execute(int fd, struct sockaddr_storage &address, void *data) {
    char response[BUF_LENGTH];
    recvSth(response, fd, BUF_LENGTH);
    reportSuccess("Received: " + string(response));
    close(fd);
}

bool CmdAskPeer::execute(int fd, struct sockaddr_storage &address, void *data) {
    reportDebug("ASKPEER", 5);
    CMDS action = ASK_HOST;
    int rand_n;
    vector<NeighborInfo> neighbors = handler->getNeighbors();
    int count = (neighbors.size() < MIN_NEIGHBOR_COUNT) ?
                neighbors.size() : MIN_NEIGHBOR_COUNT;
    struct sockaddr_storage addr;
    if (sendCmd(fd, action) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    if (DATA->config.is_superpeer) {
        count = 1;
    if (!neighbors.size()) {
        reportError("No neighbors yet");
        return false;
    }
        rand_n = rand() % neighbors.size();
        addr = neighbors.at(rand_n).address;
        if (sendSth(count, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
        if (sendSth(addr, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
    } else {
    if (!count) {
        return false;
    }
        if (sendSth(count, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
        }
        for (auto &neighbor : neighbors) {
            addr = neighbor.address;
            if (sendSth(addr, fd) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
            if (!--count)
                break;
        }
    }
    return true;
}

bool CmdAskHost::execute(int fd, struct sockaddr_storage &address, void *data) {
    reportDebug("ASKHOST", 5);
    struct sockaddr_storage addr;
    int count;
    if (recvSth(count, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
        for (int i = 0; i < count; ++i) {
            if (recvSth(addr, fd) == -1) {
                reportError("Error while communicating with peer." + MyAddr(address).get());
                return false;
            }
        addr.ss_family = AF_INET;
        if (((sockaddr_in *) &addr)->sin_port != htons(DATA->config.intValues.at("LISTENING_PORT")))
            handler->addNewNeighbor(true, addr);
    }
        reportDebug("ASKHOST END", 5);
        return true;
}

bool CmdConfirmPeer::execute(int fd, struct sockaddr_storage &address, void *data) {
    reportDebug("CONFPEER " + MyAddr(address).get() , 5);
    CMDS action = CONFIRM_HOST;
    RESPONSE_T resp = state->working ? ACK_BUSY : ACK_FREE;
    struct sockaddr_storage addr;
    try {
        addr = getHostAddr(fd);
    } catch (std::exception *e) {
        reportDebug(e->what(), 1);
    }

    /* if (DATA->IPv4_ONLY)
        addr = addr2storage("127.0.0.1",
                                                DATA->config.intValues.at("LISTENING_PORT"), AF_INET);
    else
        addr = addr2storage("127.0.0.1",
                                                DATA->config.intValues.at("LISTENING_PORT"), AF_INET6);
    */
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (sendSth(addr, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdConfirmHost::execute(int fd, struct sockaddr_storage &address, void *data) {
    reportDebug("CONFHOST", 5);
    RESPONSE_T resp;
    struct sockaddr_storage addr;
    if (recvSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportDebug("Failed to confirm neighbor " + MyAddr(address).get(), 1);
        return false;
    }
    if (recvSth(addr, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    handler->addNewNeighbor(false, addr);
    if (resp != ACK_FREE)
        handler->setNeighborFree(address, false);
    else
        handler->setNeighborFree(address, true);
    MyAddr mad(addr);
    reportDebug("Neighbor confirmed." + mad.get(), 4);
    return true;
}

bool CmdPingHost::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("PONG", 5);
    RESPONSE_T resp;
    if (recvSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp != ACK_FREE && resp != ACK_BUSY) {
        reportError("Failed to confirm neighbor " + MyAddr(address).get());
        handler->removeNeighbor(address);
        return false;
    }
    if (resp != ACK_FREE)
        handler->setNeighborFree(address, false);
    else
        handler->setNeighborFree(address, true);
    if (sendSth(DATA->config.intValues.at("LISTENING_PORT"), fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    return true;
}

bool CmdPingPeer::execute(int fd, struct sockaddr_storage &address, void *) {
    reportDebug("PING " + MyAddr(address).get() , 5);
    struct sockaddr_storage addr;
    CMDS action = PING_HOST;
    int peer_port;
    RESPONSE_T resp = state->working ? ACK_BUSY : ACK_FREE;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (recvSth(peer_port, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }
    try {
        addr = getPeerAddr(fd);
    } catch (std::exception *e) {
        reportDebug(e->what(), 1);
    }

    ((struct sockaddr_in *) &addr)->sin_port = htons(peer_port);

    if (DATA->config.is_superpeer)
        handler->addNewNeighbor(false, addr);
    else
        handler->addNewNeighbor(true, addr);
    return true;
}

bool CmdTransferPeer::execute(int fd, sockaddr_storage &address, void *) {
    CMDS action = TRANSFER_HOST;
    string fn;
    RESPONSE_T resp = state->working ? ACK_BUSY : ACK_FREE;
    if (sendCmd(fd, action) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    if (resp == ACK_FREE) {
        string answer;
        while (true) {
            answer = loadInput("", "Accept transfer?(y/n)", false);
            if ((answer == "y") || (answer == "n"))
                break;
        }
        if (answer == "n")
            resp = ACK_BUSY;
    }

    if (sendSth(resp, fd) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }

    if (resp == ACK_BUSY)
        return true;
    reportStatus("Allowing transfer. " + MyAddr(address).get());

    if (!(fn = receiveString(fd)).size()) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    reportSuccess(fn);
    return true;
}

bool CmdTransferHost::execute(int fd, sockaddr_storage &address, void *data) {
    RESPONSE_T resp;
    string fn = getBasename(*(string *) data);

    if (recvSth(resp, fd) == -1) {
        reportError("Error while communicating with peer." + MyAddr(address).get());
        return false;
    }

    if (resp == ACK_BUSY) {
        reportError("Peer is busy. " + MyAddr(address).get());
        handler->setNeighborFree(address, false);
        state->pushChunk(fn);
        return true;
    }

    if (sendString(fd, fn) == -1) {
            reportError("Error while communicating with peer." + MyAddr(address).get());
            return false;
    }
    state->to_send--;
    unique_lock<mutex> lck(state->split_mtx, defer_lock);
    lck.lock();
    lck.unlock();
    state->split_cond.notify_one();
    reportDebug("Chunk transferred. " + m_itoa(state->to_send), 1);
    return true;
}

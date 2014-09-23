#include "common.h"
#include "defines.h"
#include "commands.h"
#include "networking.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <exception>
#include <limits>
#include <string>
#include <mutex>
#include <curses.h>
#include <queue>

using namespace std;
using namespace common;

void usage() {
    printw("Usage: run [port_number]");
}

int readConfiguration(const string &cf, configuration_t &conf) {
    ifstream ifs(cf);
    string param, value, line;
    while(ifs.good()) {
        getline(ifs, line);
        stringstream ss(line);
        ss >> param;
        ss >> value;
        try {
            conf.at(param) = value;
        } catch (...) {
            reportStatus("@RUnknown parameter: " + param);
        }
    }
   return (0);
}

void cleanCommands(cmd_storage_t &cmds) {
    for (auto &c : cmds)
        delete c.second;
}

int main(int argc, char **) {
	if (argc > 2) {
		usage();
		return (1);
	}

    configuration_t conf;
    conf.insert(make_pair<string, string>("WD_PREFIX", ""));

    if (readConfiguration("conf.h", conf) == -1) {
        reportError("Error reading configuration!");
        return (1);
    }
    VideoState state(conf);
    NetworkHandle net_handler;
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(SHOW, new CmdShow(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(START, new CmdStart(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(LOAD, new CmdLoad(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(SET, new CmdSet(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(DEFCMD, new Command(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(SET_CODEC, new CmdSetCodec(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(SET_SIZE, new CmdSetChSize(&state)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(ASK, new CmdAsk(&state, 0, &net_handler)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(RESPOND, new CmdRespond(&state, 0, &net_handler)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(REACT, new CmdReact(&state, 0, &net_handler)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(CONFIRM_HOST, new CmdConfirmHost(&state, 0, &net_handler)));
    Data::getInstance()->cmds.insert(make_pair<CMDS, Command *>(CONFIRM_PEER, new CmdConfirmPeer(&state, 0, &net_handler)));
    initCurses();
    move(0,0);
    printw("Distributed video compression tool.");
    move(1, 0);
    printw("Commands: %10s%10s%10s%10s", "F6 show", "F7 start", "F8 load", "F9 set", "F12 quit");
    curs_set(0);
    std::thread thr ([&]() {
        net_handler.start_listening();
    });
    thr.detach();
    try {
        do{
        } while (!acceptCmd(Data::getInstance()->cmds));
    } catch (exception e) {
       printw(e.what());
    }
    cleanCommands(Data::getInstance()->cmds);
    endwin();
	return (0);
}

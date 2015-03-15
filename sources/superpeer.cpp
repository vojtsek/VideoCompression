
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

int main(int argc, char **) {
    if (argc > 2) {
        return (1);
    }

    configuration_t conf;
    conf.insert(make_pair<string, string>("WD_PREFIX", ""));

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
    refresh();
    struct sockaddr_storage pseudo;
    pseudo = addr2storage("111.1.7.1", 1000, AF_INET);
    net_handler.addNewNeighbor(false, pseudo);
    net_handler.addNewNeighbor(false, pseudo);
    net_handler.addNewNeighbor(false, pseudo);
    std::thread thr ([&]() {
        net_handler.start_listening(SUPERPEER_PORT);
    });
    thr.join();
    endwin();
    return (0);
}

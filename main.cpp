#include "common.h"
#include "defines.h"
#include "commands.h"
#include "networking.h"
#include "network_helper.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <exception>
#include <limits>
#include <string>
#include <mutex>
#include <queue>
#include <string.h>
#include <curses.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace common;

void usage() {
}

bool argsContains(char **argv, const char *str) {
    while (*argv != nullptr) {
        if (!strcmp(*argv, str))
            return true;
        ++argv;
    }
    return false;
}

int parseOptions(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            DATA->config.intValues.at("LISTENING_PORT") = atoi(optarg);
            break;
        case '?':
            usage();
            break;
        }
    }
    return (optind);
}

int readConfiguration(const string &cf) {
    ifstream ifs(cf);
    string param, value, line;
    int intvalue;
    while(ifs.good()) {
        getline(ifs, line);
        stringstream ss(line);
        ss >> param;
        ss >> value;
        intvalue = atoi(value.c_str());
        try {
            DATA->config.intValues.at(param) = intvalue;
        } catch (...) {
            // read also strings - determine exceptions
            reportDebug("Failed to read parameter.", 3);
        }
    }
   return (0);
}

void superPeerRoutine(NetworkHandle &net_handler) {
    net_handler.start_listening(DATA->config.intValues.at("SUPERPEER_PORT"));
}

void initConfiguration() {
    std::shared_ptr<Data> data = DATA;
    data->config.working_dir = ".";
    data->config.IPv4_ONLY = true;
    int x,y, y_space;
    getmaxyx(stdscr, y, x);
    y_space = y - 5;
    data->io_data.status_win = derwin(stdscr, y_space / 2 - 1, x, 3 + y_space / 2, 0);
    data->io_data.info_win = derwin(stdscr, y_space/ 2, 80, 3, 0);
    data->io_data.status_handler.changeWin(data->io_data.status_win);
    data->io_data.info_handler.changeWin(data->io_data.info_win);
    data->config.superpeer_addr = SUPERPEER_ADDR;
    data->config.intValues.emplace("MIN_NEIGHBOR_COUNT", MIN_NEIGHBOR_COUNT);
    data->config.intValues.emplace("LISTENING_PORT", LISTENING_PORT);
    data->config.intValues.emplace("STATUS_LENGTH", y_space / 2 - 1);
    data->config.intValues.emplace("CHUNK_SIZE", CHUNK_SIZE);
    data->config.intValues.emplace("SUPERPEER_PORT", SUPERPEER_PORT);
}

void initCommands(VideoState &state, NetworkHandle &net_handler) {
    DATA->cmds.insert(make_pair<CMDS, Command *>(DEFCMD, new CmdDef));
    DATA->cmds.insert(make_pair<CMDS, Command *>(SHOW, new CmdShow(&state, &net_handler)));
    DATA->cmds.insert(make_pair<CMDS, Command *>(START, new CmdStart(&state)));
    DATA->cmds.insert(make_pair<CMDS, Command *>(LOAD, new CmdLoad(&state)));
    DATA->cmds.insert(make_pair<CMDS, Command *>(SET, new CmdSet(&state)));
    DATA->cmds.insert(make_pair<CMDS, Command *>(SET_CODEC, new CmdSetCodec(&state)));
    DATA->cmds.insert(make_pair<CMDS, Command *>(SET_SIZE, new CmdSetChSize(&state)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(ASK, new CmdAsk(&state,  &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(RESPOND, new CmdRespond(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(REACT, new CmdReact(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(CONFIRM_HOST, new CmdConfirmHost(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(CONFIRM_PEER, new CmdConfirmPeer(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(ASK_PEER, new CmdAskPeer(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(ASK_HOST, new CmdAskHost(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(PING_PEER, new CmdPingPeer(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(PING_HOST, new CmdPingHost(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(TRANSFER_PEER, new CmdTransferPeer(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(TRANSFER_HOST, new CmdTransferHost(&state, &net_handler)));
}

void cleanCommands(cmd_storage_t &cmds) {
    for (auto &c : cmds)
        delete c.second;
}

void getNeighbors(NetworkHandle &net_handler) {
    if (net_handler.getNeighborCount() < MIN_NEIGHBOR_COUNT) {
        net_handler.obtainNeighbors(MIN_NEIGHBOR_COUNT);
    }
}

int main(int argc, char **argv) {
	if (argc > 2) {
		usage();
    }

    initCurses();
    initConfiguration();
    if (readConfiguration("CONF") == -1) {
        reportError("Error reading configuration!");
        return (1);
    }
    int optidx = parseOptions(argc, argv);
    NetworkHandle net_handler;
    VideoState state(&net_handler);
    initCommands(state, net_handler);
    WINDOW *win = subwin(stdscr, 5, 80, 0, 0);
    wmove(win, 0,0);
    wprintw(win, "Distributed video compression tool.");
    wmove(win, 1, 0);
    wprintw(win, "Commands: %10s%10s%10s%10s", "F6 show", "F7 start", "F8 load", "F9 set", "F12 quit");
    curs_set(0);
    wrefresh(win);
    refresh();
    if (argsContains(argv + optidx, string("super").c_str())) {
        DATA->config.is_superpeer = true;
        std::thread thr ([&]() {
            superPeerRoutine(net_handler);
        });
        thr.detach();
    } else {
        std::thread thr ([&]() {
            net_handler.start_listening(DATA->config.intValues.at("LISTENING_PORT"));
        });
        thr.detach();
        std::thread thr2 ([&]() {
            int sock;
            while (1) {
                net_handler.contactSuperPeer();
                getNeighbors(net_handler);
                net_handler.decrIntervals();
                for (auto n : net_handler.getNeighbors()) {
                    if (!n.intervals) {
                        reportDebug("Confirming " + MyAddr(n.address).get(), 5);
                        sock = net_handler.checkNeighbor(n.address);
                        net_handler.spawnOutgoingConnection(n.address, sock, { PING_PEER }, true, nullptr);
                    }
                }
                sleep(1);
            }
        });
        thr2.detach();
    }
    try {
        do{
        } while (!acceptCmd(DATA->cmds));
    } catch (exception e) {
       printw(e.what());
    }
    cleanCommands(DATA->cmds);
    endwin();
	return (0);
}

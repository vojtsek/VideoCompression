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
            DATA->intValues.at("LISTENING_PORT") = atoi(optarg);
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
            DATA->intValues.at(param) = intvalue;
        } catch (...) {
            // read also strings - determine exceptions
            reportDebug("Failed to read parameter.", 3);
        }
    }
   return (0);
}

void superPeerRoutine(NetworkHandle &net_handler) {
    net_handler.start_listening(DATA->intValues.at("SUPERPEER_PORT"));
}

void initConfiguration() {
    std::shared_ptr<Data> data = DATA;
    data->working_dir = ".";
    data->IPv4_ONLY = false;
    int x,y, y_space;
    getmaxyx(stdscr, y, x);
    y_space = y - 5;
    data->status_win = derwin(stdscr, y_space / 2, x, y - (y_space / 2) - 3, 0);
    data->info_win = derwin(stdscr, y_space / 2, 80, 3, 0);
    data->status_handler.changeWin(data->status_win);
    data->info_handler.changeWin(data->info_win);
    data->superpeer_addr = SUPERPEER_ADDR;
    data->intValues.emplace("NEIGHBOR_CHECK_TIMEOUT", NEIGHBOR_CHECK_TIMEOUT);
    data->intValues.emplace("MIN_NEIGHBOR_COUNT", MIN_NEIGHBOR_COUNT);
    data->intValues.emplace("LISTENING_PORT", LISTENING_PORT);
    data->intValues.emplace("STATUS_LENGTH", y_space / 2);
    data->intValues.emplace("CHUNK_SIZE", CHUNK_SIZE);
    data->intValues.emplace("SUPERPEER_PORT", SUPERPEER_PORT);
}

void initCommands(VideoState &state, NetworkHandle &net_handler) {
    std::shared_ptr<Data> data = DATA;
    data->cmds.insert(make_pair<CMDS, Command *>(SHOW, new CmdShow(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(START, new CmdStart(&state)));
    data->cmds.insert(make_pair<CMDS, Command *>(LOAD, new CmdLoad(&state)));
    data->cmds.insert(make_pair<CMDS, Command *>(SET, new CmdSet(&state)));
    data->cmds.insert(make_pair<CMDS, Command *>(DEFCMD, new Command(&state)));
    data->cmds.insert(make_pair<CMDS, Command *>(SET_CODEC, new CmdSetCodec(&state)));
    data->cmds.insert(make_pair<CMDS, Command *>(SET_SIZE, new CmdSetChSize(&state)));
    data->cmds.insert(make_pair<CMDS, Command *>(ASK, new CmdAsk(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(RESPOND, new CmdRespond(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(REACT, new CmdReact(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(CONFIRM_HOST, new CmdConfirmHost(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(CONFIRM_PEER, new CmdConfirmPeer(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(ASK_PEER, new CmdAskPeer(&state, 0, &net_handler)));
    data->cmds.insert(make_pair<CMDS, Command *>(ASK_HOST, new CmdAskHost(&state, 0, &net_handler)));
}

void cleanCommands(cmd_storage_t &cmds) {
    for (auto &c : cmds)
        delete c.second;
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
    VideoState state;
    NetworkHandle net_handler;
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
        DATA->is_superpeer = true;
        superPeerRoutine(net_handler);
    }
    std::thread thr ([&]() {
        net_handler.start_listening(DATA->intValues.at("LISTENING_PORT"));
    });
    thr.detach();
    std::thread thr2 ([&]() {
        for (;;) {
            if (net_handler.getNeighborCount() < MIN_NEIGHBOR_COUNT)
                net_handler.confirmNeighbors(MIN_NEIGHBOR_COUNT);
            sleep(DATA->intValues.at("NEIGHBOR_CHECK_TIMEOUT"));
       }
    });
    thr2.detach();
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

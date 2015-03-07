#include "include_list.h"
#include "defines.h"
#include "commands.h"
#include "networkhandler.h"
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
using namespace utilities;

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

int readConfiguration(const std::string &cf) {
    ifstream ifs(cf);
    std::string param, value, line;
    int intvalue;
    while(ifs.good()) {
        getline(ifs, line);
        std::stringstream ss(line);
        ss >> param;
        ss >> value;
        intvalue = atoi(value.c_str());
        try {
            DATA->config.intValues.insert(make_pair(param, intvalue));
        } catch (...) {
            // read also std::strings - determine exceptions
            reportError("Failed to read parameter " + param);
        }
    }
   return (0);
}

void superPeerRoutine(NetworkHandler &net_handler) {
    net_handler.start_listening(DATA->config.intValues.at("SUPERPEER_PORT"));
}

void initConfiguration() {
    std::shared_ptr<Data> data = DATA;
    DATA->state.can_accept = DATA->config.getValue("MAX_ACCEPTED_CHUNKS");
    DATA->state.to_send = 0;
    data->config.working_dir = WD;
    if (data->config.working_dir == "") {
        char dirp[BUF_LENGTH];
        if (getcwd(dirp, BUF_LENGTH) == NULL)
            data->config.working_dir = std::string(dirp);
        else
            data->config.working_dir = ".";
    }
    data->config.IPv4_ONLY = true;
    int x,y, y_space;
    getmaxyx(stdscr, y, x);
    y_space = y - 5;
    data->io_data.status_win = derwin(stdscr, y_space / 2 - 1, x, 3 + y_space / 2, 0);
    data->io_data.info_win = derwin(stdscr, y_space/ 2, 80, 3, 0);
    data->io_data.status_handler.changeWin(data->io_data.status_win);
    data->io_data.info_handler.changeWin(data->io_data.info_win);
    data->config.superpeer_addr = SUPERPEER_ADDR;
    data->config.intValues.emplace("STATUS_LENGTH", y_space / 2 - 1);
}

void initCommands(VideoState &state, NetworkHandler &net_handler) {
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
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(DISTRIBUTE_PEER, new CmdDistributePeer(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(DISTRIBUTE_HOST, new CmdDistributeHost(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(RETURN_PEER, new CmdReturnPeer(&state, &net_handler)));
    DATA->net_cmds.insert(make_pair<CMDS, NetworkCommand *>(RETURN_HOST, new CmdReturnHost(&state, &net_handler)));
}

void cleanCommands(cmd_storage_t &cmds) {
    for (auto &c : cmds)
        delete c.second;
}

void periodicActions(NetworkHandler &net_handler) {
    if (net_handler.getNeighborCount() < DATA->config.getValue("MIN_NEIGHBOR_COUNT"))
        net_handler.obtainNeighbors(DATA->config.getValue("MIN_NEIGHBOR_COUNT"));
    net_handler.contactSuperPeer();
    for (auto l : DATA->periodic_listeners) {
        l.second->invoke(net_handler);
    }
    //applyToVector(DATA->periodic_listeners, [=](Listener *obj) { obj->invoke(net_handler); });
}

int main(int argc, char **argv) {
	if (argc > 2) {
		usage();
    }

    initCurses();
    if (readConfiguration("CONF") == -1) {
        reportError("Error reading configuration!");
        return (1);
    }
    initConfiguration();
    int optidx = parseOptions(argc, argv);
    NetworkHandler net_handler;
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
    if (argsContains(argv + optidx, std::string("super").c_str())) {
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
            while (1) {
                periodicActions(net_handler);
                sleep(1);
            }
        });
        thr2.detach();
        std::thread thr3(chunkProcessRoutine);
        thr3.detach();
        std::thread split_thr ([&]() {
            chunkSendRoutine(&net_handler);
        });
        split_thr.detach();
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

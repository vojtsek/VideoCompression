#include "headers/include_list.h"
#include "headers/defines.h"
#include "structures/NetworkHandler.h"

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
#include <signal.h>

using namespace std;
using namespace utilities;

void usage() {
    return;
}

bool argsContains(char **argv, const char *str) {
    // traverses the arguments and compares
    while (*argv != nullptr) {
        if (!strcmp(*argv, str)) {
            return true;
        }
        ++argv;
    }
    return false;
}

int64_t parseOptions(int64_t argc, char **argv) {
    int opt;
    // use of getopt
    while ((opt = getopt(argc, argv, "sc:p:d:")) != -1) {
        switch (opt) {
        // listening port set
        case 'p':
            DATA->config.intValues.emplace("LISTENING_PORT", atoi(optarg));
            break;
        // superpeer mode
        case 's':
            DATA->config.is_superpeer = true;
            break;
        // debug level set
        case 'd':
            DATA->config.debug_level = atoi(optarg);
            break;
        // port to listen superpeer
        case 'c':
            DATA->config.intValues.emplace("SUPERPEER_PORT", atoi(optarg));
        // unknown option
        case '?':
            usage();
            break;
        }
    }
    // index of last parameter
    return (optind);
}

int64_t readConfiguration(const std::string &cf) {
    // config file
    ifstream ifs(cf);
    std::string param_name, value, line;
    int64_t intvalue;
    while(ifs.good()) {
        // reads line from the file
        getline(ifs, line);
        std::stringstream ss(line);
        // reads the line splitted
        ss >> param_name;
        ss >> value;
        try {
            // tries to read the integer
            intvalue = std::stoi(value);
            try {
                // if integer was parsed, add to mapping
                DATA->config.intValues.insert(
                            std::make_pair(param_name, intvalue));
            } catch (...) {
                reportError("Failed to read parameter " + param_name);
            }
        } catch (std::invalid_argument) {
            // otherwise handles the value as string
            try {
                DATA->config.strValues.insert(
                            std::make_pair(param_name, value));
            } catch (...) {
                reportError("Failed to read parameter " + param_name);
            }
        }
    }
   return 0;
}

void superPeerRoutine(NetworkHandler &net_handler) {
    // spawns listening routine
    net_handler.start_listening(DATA->config.intValues.at("SUPERPEER_PORT"));
}

void initConfiguration(NetworkHandler &handler) {
    // data initialized with default values
    // called after the config file was read
    DATA->state.can_accept = DATA->config.getIntValue("MAX_ACCEPTED_CHUNKS");
    if (DATA->config.getStringValue("WD") == "") {
        // if the WD is not determined, use current working dir
        char dirp[BUF_LENGTH];
        if (getcwd(dirp, BUF_LENGTH) == NULL) {
            DATA->config.strValues.emplace("WD",
                                           std::string(dirp));
        } else {
            DATA->config.strValues.emplace("WD", ".");
        }
    }
    DATA->config.intValues.emplace("UPP_LIMIT", 8);
    DATA->config.IPv4_ONLY = false;
    int64_t x,y, y_space;
    // dimensions of the window
    getmaxyx(stdscr, y, x);
    // number of vertical rows that can be used
    y_space = y - 5;
    // curses window initialization
    DATA->io_data.status_win = derwin(stdscr, y_space / 2 - 1, x, 3 + y_space / 2, 0);
    DATA->io_data.info_win = derwin(stdscr, y_space/ 2, x, 3, 0);
    DATA->io_data.status_handler.changeWin(DATA->io_data.status_win);
    DATA->io_data.info_handler.changeWin(DATA->io_data.info_win);
    DATA->io_data.changeLogLocation(DATA->config.getStringValue("WD") + "/log_" +
                                    utilities::getTimestamp() + "_" +
                                    utilities::m_itoa(DATA->config.getIntValue("LISTENING_PORT")));
    DATA->io_data.info_handler.setLength(6);
    DATA->io_data.status_handler.setLength(y_space / 2);

    // obtain with which address is communicating
    struct sockaddr_storage addr, super_addr;
    networkHelper::getSuperPeerAddr(super_addr);
    networkHelper::getMyAddress(super_addr,
                addr, &handler);
    DATA->config.my_IP = MyAddr(addr);
     DATA->config.my_IP.port =
             DATA->config.getIntValue("LISTENING_PORT");
}

void initCommands(VideoState &state, NetworkHandler &net_handler) {
    DATA->cmds.emplace(CMDS::DEFCMD, new CmdDef(&state));
    DATA->cmds.emplace(CMDS::SHOW, new CmdShow(&state, &net_handler));
    DATA->cmds.emplace(CMDS::START, new CmdStart(&state));
    DATA->cmds.emplace(CMDS::LOAD, new CmdLoad(&state));
    DATA->cmds.emplace(CMDS::SET, new CmdSet(&state));
    DATA->cmds.emplace(CMDS::ABORT_C, new CmdAbort(&state));
    DATA->cmds.emplace(CMDS::SET_CODEC, new CmdSetCodec(&state));
    DATA->cmds.emplace(CMDS::SET_SIZE, new CmdSetChSize(&state));
    DATA->cmds.emplace(CMDS::SET_FORMAT, new CmdSetFormat(&state));
    DATA->cmds.emplace(CMDS::SCROLL_DOWN, new CmdScrollDown(&state));
    DATA->cmds.emplace(CMDS::SCROLL_UP, new CmdScrollUp(&state));
    DATA->net_cmds.emplace(CMDS::CONFIRM_HOST, new CmdConfirmHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::CONFIRM_PEER, new CmdConfirmPeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::ASK_PEER, new CmdAskPeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::ASK_HOST, new CmdAskHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::PING_PEER, new CmdPingPeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::PING_HOST, new CmdPingHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::DISTRIBUTE_PEER, new CmdDistributePeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::DISTRIBUTE_HOST, new CmdDistributeHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::GATHER_PEER, new CmdGatherNeighborsPeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::GATHER_HOST, new CmdGatherNeighborsHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::RETURN_PEER, new CmdReturnPeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::RETURN_HOST, new CmdReturnHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::GOODBYE_PEER, new CmdGoodbyePeer(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::GOODBYE_HOST, new CmdGoodbyeHost(&state, &net_handler));
    DATA->net_cmds.emplace(CMDS::SAY_GOODBYE, new CmdSayGoodbye(&state, &net_handler));
}

void cleanCommands(cmd_storage_t &cmds) {
    // free the memory
    for (auto &c : cmds) {
        delete c.second;
    }
}

void periodicActions(NetworkHandler &net_handler, VideoState *state) {
    // check whether should gain some neighbors
    if (DATA->neighbors.getNeighborCount() < DATA->config.getIntValue(
                "MAX_NEIGHBOR_COUNT")) {
        net_handler.obtainNeighbors();
    }
    // invoke the listeners
    DATA->periodic_listeners.applyTo(
                [&](Listener *l) { l->invoke(net_handler);  });
    // in case that some neighbors were not able to check
    DATA->neighbors.removeDirty();
    utilities::printOverallState(state);
}

int main(int argc, char **argv) {
	if (argc > 2) {
		usage();
    }

    // handle the options
    parseOptions(argc, argv);
    // inits the curses variables
    initCurses();
    if (readConfiguration("CONF") == -1) {
        reportError("Error reading configuration!");
        return 1;
    }

    NetworkHandler net_handler;
    // inits the configuration
    initConfiguration(net_handler);
    // prepares the working directory
    DATA->config.strValues.at("WD") = DATA->config.getStringValue("WD") + "/" +
           utilities::m_itoa(DATA->config.getIntValue("LISTENING_PORT"));
    if (OSHelper::prepareDir(
                DATA->config.getStringValue("WD"), false) == -1) {
        reportError("Failed to prepare working directory.");
        return 1;
    }
    VideoState state(&net_handler);

    // sets handle of SIGPIPE
    struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = &networkHelper::sigPipeHandler;
        sigaction(SIGPIPE, &sa, NULL);

    // creates the commands structures
    initCommands(state, net_handler);
    // initialize the main window
    WINDOW *win = subwin(stdscr, 5, 80, 0, 0);
    wmove(win, 0,0);
    wprintw(win, "Distributed video compression tool.");
    wmove(win, 1, 0);
    wprintw(win, "Commands: %10s%10s%10s%10s%10s%10s", "F6 show",
            "F7 start", "F8 load", "F9 set",
            "F10 Abort", "F12 quit");
    curs_set(0);
    wrefresh(win);
    refresh();
    if (DATA->config.is_superpeer) {
        // starts listening at the super peer port
        std::thread thr ([&]() {
            superPeerRoutine(net_handler);
        });
        thr.detach();
    } else {
        //TODO mechanism to propagate fail outside of thread
        std::thread thr ([&]() {
                    // ping the super peer
                    net_handler.contactSuperPeer();
                    // creates the socket, binds and starts listening
                    net_handler.start_listening(
                                                DATA->config.intValues.at("LISTENING_PORT"));
        });
        thr.detach();
        std::thread thr2 ([&]() {
            // invokes some action periodically
            while (1) {
                periodicActions(net_handler, &state);
                sleep(TICK_DURATION);
            }
        });
        thr2.detach();
        // waits for chunk to receive and be processed
        std::thread thr3(chunkhelper::chunkProcessRoutine);
        thr3.detach();
        // waits for chunks to be sent
        std::thread split_thr ([&]() {
            chunkhelper::chunkSendRoutine(&net_handler);
        });
        split_thr.detach();
    }
    // loop and tries read command keys
    // acceptCmd fails on F12
    try {
        do{
        } while (!acceptCmd(DATA->cmds));
    } catch (exception e) {
       printw(e.what());
    }
    // notify neighbors
    DATA->net_cmds.at(CMDS::SAY_GOODBYE)->execute();
    // clear memory
    cleanCommands(DATA->cmds);
    // handles curses end
    endwin();
	return (0);
}

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

void cleanCommands() {
    // free the memory
    for (auto &c : DATA->cmds) {
        delete c.second;
    }
    for (auto &c : DATA->net_cmds) {
        delete c.second;
    }
}

void exitProgram(const std::string &msg, int64_t retval) {
    try {
            // notify neighbors
            DATA->net_cmds.at(CMDS::SAY_GOODBYE)->execute();
    } catch (std::out_of_range) {}
    // clear memory
    //TODO: segfault
    //cleanCommands();
    // handles curses end
    endwin();
    printf("%s\n", msg.c_str());
    exit(retval);
}

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
/*
 * -s ...run as superpeer
 * -c address:port_number ...node to contact if no neighbors
 * -p port ...listenning port
 * -d level ...debug level
 * -t ...test, i.e. encode the loaded file first and measure the time
 */

    int opt;
    int64_t port;
    std::string addr_port, address;
    // use of getopt
    while ((opt = getopt(argc, argv, "tsi:c:p:d:")) != -1) {
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
            addr_port = std::string(optarg);
            address = addr_port.substr(
                        0, addr_port.find('~'));
            port = atoi(
                        addr_port.substr(addr_port.find('~') + 1).c_str());
            DATA->config.strValues.emplace("SUPERPEER_ADDR", address);
            DATA->config.intValues.emplace("SUPERPEER_PORT", port);
            break;
        case 'i':
            addr_port = std::string(optarg);
            address = addr_port.substr(
                        0, addr_port.find('~'));
            port = atoi(
                        addr_port.substr(addr_port.find('~') + 1).c_str());
            DATA->config.strValues.emplace("MY_IP", address);
            DATA->config.intValues.emplace("LISTENING_PORT", port);
            break;
        case 't':
            DATA->config.encode_first = true;
            break;
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
    if (!OSHelper::isFileOk(cf)) {
        return -1;
    }
    // config file
    ifstream ifs(cf);
    if (!ifs.is_open()) {
        return -1;
    }
    std::string param_name, value, line;
    int64_t intvalue;
    while(ifs.good()) {
        // reads line from the file
        getline(ifs, line);
        std::stringstream ss(line);
        // reads the line splitted
        ss >> param_name;
        // ignore comments
        if (*param_name.begin() == '#') {
            continue;
        }
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
    // TODO: essential values check
    // data initialized with default values
    // called after the config file was read
    // from config file
    std::string wdir = DATA->config.getStringValue("WD");
    if (wdir == "") {
        // if the WD is not determined, use current working dir
        char dirp[BUF_LENGTH];
        if (getcwd(dirp, BUF_LENGTH) == NULL) {
            wdir = std::string(dirp);
        } else {
            wdir = ".";
        }
    }
    wdir += PATH_SEPARATOR + utilities::m_itoa(
                DATA->config.getIntValue("LISTENING_PORT"));
    if (OSHelper::prepareDir(
                wdir, false) == -1) {
        exitProgram("Failed to prepare working directory.", 1);
    }

    if (OSHelper::prepareDir(
                wdir + PATH_SEPARATOR + "logs", false) == -1) {
        exitProgram("Failed to prepare working directory.", 1);
    }

    try {
            DATA->config.strValues.at("WD") = wdir;
    } catch (std::out_of_range) {
        DATA->config.strValues.emplace("WD", wdir);
    }

    // how many neighbors contact when gathering
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
    DATA->io_data.changeLogLocation(LOG_PATH + PATH_SEPARATOR +
                                    utilities::getTimestamp() + ".log");
    DATA->io_data.info_handler.setLength(8);
    DATA->io_data.status_handler.setLength(y_space / 2);

    // obtain with which address is communicating
    struct sockaddr_storage addr, super_addr;
    networkHelper::getSuperPeerAddr(super_addr);
    if (networkHelper::getMyAddress(super_addr,
                addr, &handler) != -1) {
        DATA->config.my_IP = MyAddr(addr);
    } else {
        exitProgram("Failed to obtain IP address.", 1);
    }
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
    DATA->cmds.emplace(CMDS::SET_QUALITY, new CmdSetQuality(&state));
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
    // handle the options
    parseOptions(argc, argv);
    // inits the curses variables
    initCurses();
    if (readConfiguration("CONF") == -1) {
        exitProgram("Failed to read configuration.", 1);
    }

    NetworkHandler net_handler;
    // inits the configuration
    initConfiguration(net_handler);
    // prepares the working directory
    VideoState state(&net_handler);

    // sets handler of SIGPIPE
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
        std::thread thr ([&]() {
                    // ping the super peer
                    net_handler.contactSuperPeer();
                    // creates the socket, binds and starts listening
                    net_handler.start_listening(
                                                DATA->config.intValues.at("LISTENING_PORT"));
                    exitProgram("Failed to bind.", 2);
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
    exitProgram("", 0);
}

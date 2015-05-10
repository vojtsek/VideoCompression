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
 * -n address:port_number ...node to contact if no neighbors
 * -h directory ...path to the home directory
 * -i file file to encode
 * -p port ...listenning port
 * -d level ...debug level
 * -t ...test, i.e. encode the loaded file first and measure the time
 */

    int opt;
    int64_t port;
    std::string addr_port, address;
    // use of getopt
    while ((opt = getopt(argc, argv, "sq:a:h:i:d:n:")) != -1) {
        switch (opt) {
        // superpeer mode
        case 's':
            DATA->config.is_superpeer = true;
            break;
        // debug level set
        case 'd':
            DATA->config.debug_level = atoi(optarg);
            break;
        // port to listen superpeer
        case 'n':
            addr_port = std::string(optarg);
            address = addr_port.substr(
                        0, addr_port.find('~'));
            port = atoi(
                        addr_port.substr(addr_port.find('~') + 1).c_str());
            DATA->config.setValue("SUPERPEER_ADDR", address, false);
            DATA->config.setValue("SUPERPEER_PORT", port, false);
            break;
            // host address
        case 'a':
            addr_port = std::string(optarg);
            address = addr_port.substr(
                        0, addr_port.find('~'));
            port = atoi(
                        addr_port.substr(addr_port.find('~') + 1).c_str());
            DATA->config.setValue("MY_IP", address, false);
            DATA->config.setValue("LISTENING_PORT", port, false);
            break;
        // configuration file location
        case 'h':
            DATA->config.setValue("WD", std::string(optarg), true);
            DATA->state.wd_provided = true;
            break;
                // file to load
        case 'i':
            DATA->state.file_path = std::string(optarg);
            break;
        case 'q':
            DATA->config.setValue("QUALITY", std::string(optarg), true);
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
    int64_t int_value;
    while(ifs.good()) {
        // reads line from the file
        getline(ifs, line);
        std::stringstream ss(line);
        // reads the line splitted
        ss >> param_name;
        // ignore comments
        if ((param_name.size() > 0) &&
                (param_name.at(0) == '#')) {
            continue;
        }
        ss >> value;
                // handles the value as string and saves
                try {
                        DATA->config.setValue(
                                                param_name, value, false);
                } catch (...) {
                        reportError("Failed to read parameter " + param_name);
                }
        try {
            // tries to read the integer
            int_value = std::stoi(value);
            try {
                // if integer was parsed, add to mapping
                DATA->config.setValue(
                            param_name, int_value, false);
            } catch (...) {
                reportError("Failed to read parameter " + param_name);
            }
        } catch (std::invalid_argument) {

        }
    }
   return 0;
}

void initConfiguration(NetworkHandler &handler) {
    // data initialized with default values
    // called after the config file was read
    // from config file

    if (readConfiguration(DATA->config.location) == -1) {
        utilities::exitProgram("Failed to read configuration.", 1);
    }

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
    // sets the WDIR if not provided by option
    DATA->config.setValue("WD", wdir,
                          !DATA->state.wd_provided);
    // really used one
    wdir = DATA->config.getStringValue("WD");
    if (OSHelper::prepareDir(
                wdir, false) == -1) {
        utilities::exitProgram("Failed to prepare working directory.", 1);
    }

    if (OSHelper::prepareDir(
                wdir + PATH_SEPARATOR + "logs", false) == -1) {
        utilities::exitProgram("Failed to prepare working directory.", 1);
    }

    // how many neighbors contact when gathering
    DATA->config.setValue("UPP_LIMIT", 8, false);
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
        utilities::exitProgram("Failed to obtain IP address.", 1);
    }
     DATA->config.my_IP.port =
             DATA->config.getIntValue("LISTENING_PORT");
}

void initCommands(TaskHandler &state, NetworkHandler &net_handler) {
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

void periodicActions(NetworkHandler &net_handler) {
    // check whether should gain some neighbors
    if (
            (DATA->neighbors.getNeighborCount() < DATA->config.getIntValue(
                "MAX_NEIGHBOR_COUNT"))) {
        net_handler.obtainNeighbors();
    }
    // invoke the listeners
    DATA->periodic_listeners.applyTo(
                [&](Listener *l) { l->invoke(net_handler);  });
    // in case that some neighbors were not able to check
    DATA->neighbors.removeDirty();
}

void drawCurses(WINDOW *win) {
    // initialize the main window
    keypad(win, TRUE);
    wmove(win, 0,0);
    wprintw(win, "Distributed video compression tool.");
    wmove(win, 1, 0);
    wprintw(win, "Commands: %10s%10s%10s%10s%10s%10s", "F6 show",
            "F7 start", "F8 load", "F9 set",
            "F10 Abort", "F12 quit");
    curs_set(0);
    wrefresh(win);
}

int main(int argc, char **argv) {
    // handle the options
    DATA->config.location = "CONF";
    parseOptions(argc, argv);
    if (argsContains(argv, "ipv4")) {
        DATA->config.IPv4_ONLY = true;

    }
    WINDOW *win;
    if (argsContains(argv, "nostdin")) {
        reportError("DD");
        DATA->state.interact = false;
    } else {
            // inits the curses variables
            initCurses();
                    win = subwin(stdscr, 5, 80, 0, 0);
            drawCurses(win);
    }

    NetworkHandler net_handler;
    // inits the configuration
    initConfiguration(net_handler);
    // prepares the working directory
    TaskHandler state(&net_handler);


    // sets handler of SIGPIPE
    struct sigaction sa, sa2;
        memset(&sa, 0, sizeof(struct sigaction));
        memset(&sa2, 0, sizeof(struct sigaction));

        sa.sa_handler = &networkHelper::sigPipeHandler;
        sigaction(SIGPIPE, &sa, NULL);

        sa2.sa_handler = &utilities::sigQuitHandler;
        sigaction(SIGINT, &sa2, NULL);

    // creates the commands structures
    initCommands(state, net_handler);
        std::thread thr ([&]() {
                                    if (!DATA->config.is_superpeer) {
                    // ping the super peer
                    net_handler.contactSuperPeer();
                                    }
                    // creates the socket, binds and starts listening
                    if (net_handler.start_listening(
                                                DATA->config.getIntValue("LISTENING_PORT")) == -1) {
                                            utilities::exitProgram("Failed to bind.", 2);
                    }
        });
        thr.detach();
        std::thread thr2 ([&]() {
            // invokes some action periodically
            while (1) {
                periodicActions(net_handler);
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
    if (DATA->state.file_path != "") {
            if (state.loadFile(
                                    DATA->state.file_path) == -1) {
                    reportError("Failed to load file!");
            }
    }
    if (!DATA->state.interact) {
        // no interaction -> begin process
            std::unique_lock<std::mutex> lck(DATA->m_data.interact_mtx, std::defer_lock);
            lck.lock();
            if (DATA->state.file_path != "") {
                            if (state.split() == -1) {
                utilities::exitProgram("Failed to split", 1);
                            }
            } else {
                // hack to "get stuck" when should only listen
                DATA->state.working = true;
                DATA->config.serve_while_working = true;
            }
            while (DATA->state.working) {
                    DATA->m_data.interact_cond.wait(lck);
            }
            lck.unlock();
    } else {
    // loops and tries read command keys
    // acceptCmd fails on F12
        try {
                    do{ } while (!utilities::acceptCmd(DATA->cmds, win));
        } catch (exception e) {
                    printw(e.what());
                }
    }
    utilities::exitProgram("", 0);
}

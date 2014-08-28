#include "common.h"
#include "defines.h"
#include "commands.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <exception>
#include <limits>
#include <curses.h>

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
            reportError("Unknown parameter: " + param);
        }
    }
   return (0);
}

void cleanCommands(cmd_storage_t &cmds) {
    for (auto &c : cmds)
        delete c.second;
}

int main(int argc, char ** argv) {
	if (argc > 2) {
		usage();
		return (1);
	}

    cmd_storage_t cmds;
    configuration_t conf;
    conf.insert(make_pair<string, string>("WD_PREFIX", ""));

    if (readConfiguration("conf.h", conf) == -1) {
        reportError("Error reading configuration!");
        return (1);
    }
    State state(conf);
    cmds.insert(make_pair<string, Command *>("show", new CmdShow));
    cmds.insert(make_pair<string, Command *>("start", new CmdStart));
    cmds.insert(make_pair<string, Command *>("help", new CmdHelp));
    cmds.insert(make_pair<string, Command *>("set", new CmdSet));
    cmds.insert(make_pair<string, Command *>("load", new CmdLoad));
    cmds.insert(make_pair<string, Command *>("default", new Command));

//    int c;
    initCurses();
    char line[80];
    stringstream ss;
    try {
        do{
            printw(">");
            getstr(line);
            ss.clear();
            ss.str(line);
        } while (!readCmd(ss, cmds, state));
    } catch (exception e) {
       printw(e.what());
    }

    cleanCommands(cmds);
    endwin();
	return (0);
}

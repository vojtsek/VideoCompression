#include "common.h"
#include "defines.h"
#include "commands.h"

#include <iostream>
#include <map>
#include <exception>
#include <limits>
#include <curses.h>

using namespace std;
using namespace common;

void usage() {
	cout << "Usage: run [port_number]" << endl;
}

void cleanCommands(map<string, Command *> &cmds) {
    for (auto &c : cmds)
        delete c.second;
}

int main(int argc, char ** argv) {
	if (argc > 2) {
		usage();
		return (1);
	}

    map<string, Command *> cmds;
    State state;
    cmds.insert(make_pair<string, Command *>("show", new CmdShow));
    cmds.insert(make_pair<string, Command *>("start", new CmdStart));
    cmds.insert(make_pair<string, Command *>("help", new CmdHelp));
    cmds.insert(make_pair<string, Command *>("set", new CmdSet));
    cmds.insert(make_pair<string, Command *>("load", new CmdLoad));
    cmds.insert(make_pair<string, Command *>("default", new Command));

//    int c;
//    initscr();
//    cbreak();
//    keypad(stdscr, TRUE);
    try {
        do{
            cout.put('>');
        } while (!readCmd(cin, cmds, state));
    } catch (exception e) {
        printInfo(string(e.what()));
    }

    cleanCommands(cmds);
//    endwin();
	return (0);
}

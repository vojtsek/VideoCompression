#ifndef WINDOWPRINTER_H
#define WINDOWPRINTER_H

#include "headers/enums_types.h"
#include <fstream>
#include <deque>
#include <curses.h>

class WindowPrinter {
    WINDOW *win;
    std::deque<printable_pair_T> q;
    bool bolded;
    std::ofstream ofs;
public:
    PRINTER_DIRECTION direction;
    PRINTER_START start;
    WindowPrinter(PRINTER_DIRECTION dir, bool b,
                  PRINTER_START st, std::string log_location):
        bolded(b), ofs(log_location), direction(dir), start(st) {
        win = stdscr;
    }
    void changeWin(WINDOW *nwin);
    int add(std::string msg, MSG_T type);
    void updateAt(int idx, std::string value, MSG_T type);
    void changeLogLocation(std::string log_location);
    void clear();
    void print();
};

#endif // WINDOWPRINTER_H

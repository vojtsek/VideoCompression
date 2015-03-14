#ifndef WINDOWPRINTER_H
#define WINDOWPRINTER_H

#include "enums_types.h"
class WindowPrinter {
    WINDOW *win;
    std::deque<printable_pair_T> q;
    bool bolded;
public:
    PRINTER_DIRECTION direction;
    PRINTER_START start;
    WindowPrinter(PRINTER_DIRECTION dir, bool b, PRINTER_START st):
        bolded(b), direction(dir), start(st) {
        win = stdscr;
    }
    void changeWin(WINDOW *nwin);
    int add(std::string msg, MSG_T type);
    void updateAt(int idx, std::string value, MSG_T type);
    void clear();
    void print();
};

#endif // WINDOWPRINTER_H

#define _FILE_OFFSET_BITS 64
#include "include_list.h"
#include "handle_IO.h"
#include "commands.h"
#include "defines.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <curses.h>
#include <deque>
#include <thread>
#include <string.h>
#include <sys/stat.h>

using namespace std;

string loadInput(const std::string &histf, const std::string &msg, bool save) {
    std::unique_lock<std::mutex> lck(DATA->m_data.I_mtx, std::defer_lock);
    std::unique_lock<std::mutex> lck2(DATA->m_data.O_mtx, std::defer_lock);

    lck.lock();
    lck2.lock();
    DATA->m_data.using_O = true;
    cursToQuestion();
    printw("%s", msg.c_str());
    char line[LINE_LENGTH];
    curs_set(0);
    cursToCmd();
    printw(">");
    refresh();
    DATA->m_data.using_O = false;
    lck2.unlock();

    DATA->m_data.using_I = true;
    getLine(line, LINE_LENGTH, histf, save);
    curs_set(0);
    cursorToX(0);
    clrtoeol();
    cursToQuestion();
    clrtoeol();
    refresh();
    DATA->m_data.using_I = false;
    lck.unlock();
    DATA->m_data.IO_cond.notify_one();
    return std::string(line);
}

void reportFileProgress(const std::string &file, long desired) {
    long fs = 0, old = 0;
    int tries = 10;
    double percent;
    while (fs < desired) {
        fs = utilities::getFileSize(file);
        if (fs < 0)
            fs = 0;
        if (fs == old) {
            if (fs > 0)
                --tries;
            usleep(150000);
        } else
            tries = 10;
        if (!tries)
            break;
        old = fs;
        percent = (double) fs / (double) desired;
        printProgress(percent);
        usleep(10000);
    }
}

void reportError(const std::string &err) {
    DATA->io_data.status_handler.add(err, ERROR);
    DATA->io_data.status_handler.print();
}

void reportSuccess(const std::string &msg) {
    DATA->io_data.status_handler.add(msg, SUCCESS);
    DATA->io_data.status_handler.print();
}

void reportStatus(const std::string &msg) {
    DATA->io_data.status_handler.add(msg, PLAIN);
    DATA->io_data.status_handler.print();
}

void reportDebug(const std::string &msg, int lvl) {
    if (lvl <= DEBUG_LEVEL) {
        DATA->io_data.status_handler.add(msg, DEBUG);
        DATA->io_data.status_handler.print();
    }
}

void initCurses() {
    initscr();
    keypad(stdscr, TRUE);
    signal (SIGWINCH, NULL);
    noecho();
    halfdelay(3);
    start_color();
    init_color(COLOR_GREY, 350, 350, 350);
    init_color(COLOR_CYAN, 500, 500, 1000);
    init_pair(RED, COLOR_RED, BG_COL);
    init_pair(GREEN, COLOR_GREEN, BG_COL);
    init_pair(BLUE, COLOR_BLUE, BG_COL);
    init_pair(YELLOWALL, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(CYANALL, COLOR_CYAN, COLOR_CYAN);
    init_pair(GREYALL, COLOR_GREY, COLOR_GREY);
    init_pair(BG, COLOR_WHITE, BG_COL);
    init_pair(INVERTED, BG_COL, COLOR_WHITE);
    wbkgd(stdscr, COLOR_PAIR(BG));
}

void printProgress(double percent) {
    unique_lock<mutex> lck(DATA->m_data.O_mtx, defer_lock);
    lck.lock();
    DATA->m_data.using_O = true;
    cursToPerc();
    clrtoeol();
    printw("(%d%%)", (int) (percent * 100));
    attron(COLOR_PAIR(CYANALL));
    for(int i = 0; i < percent * (getmaxx(stdscr) - 7); ++i)
        printw("#");
    attroff(COLOR_PAIR(CYANALL));
    refresh();
    DATA->m_data.using_O = false;
    lck.unlock();
}

void cursToCmd() {
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);
    move(max_y - 1, 0);
    clrtoeol();
}

void cursToInfo() {
    move(3, 0);
}

//TODO: still causes occasional problems
void cursToStatus() {
    if (!DATA->io_data.status_y) {
        DATA->io_data.status_y = getmaxy(stdscr) - 4;
    }
    static int status_y = DATA->io_data.status_y;
    move(status_y, 0);
}

void cursToQuestion() {
    if (!DATA->io_data.question_y)
        DATA->io_data.question_y = getmaxy(stdscr) - 2;
    move(DATA->io_data.question_y, 0);
}

void cursToPerc() {
    if (!DATA->io_data.perc_y)
        DATA->io_data.perc_y = getmaxy(stdscr) - 3;
    move(DATA->io_data.perc_y, 0);
}

void clearProgress() {
    cursToPerc();
    clrtoeol();
}

void cursorToX(int nx) {
    int y, x;
    getyx(stdscr, y, x);
    move(y, nx);
}

void reportTime(const std::string &file, double time) {
    std::string fn("results/" + utilities::getBasename(file) + ".measured");
    ofstream out;
    out.open(fn, ofstream::app);
    std::stringstream msg;
    out << time << endl;
    out.close();
    msg << "The operation took " << time << " seconds.";
    reportStatus(msg.str());
}

int getLine(char *line, int len, const std::string &histf, bool save) {
    HistoryStorage hist(histf);
    char *start = line;
    *line = '\0';

    nocbreak();
    cbreak();
    wchar_t c;
    int read = 0;
    while(++read <= len) {
        c = getch();
        if (c == KEY_UP) {
            cursorToX(1);
            clrtoeol();
            hist.prev();
            try {
                printw(hist.getCurrent().c_str());
                read = hist.getCurrent().size();
                strncpy(start, hist.getCurrent().c_str(), read);
                line = start + read;
            } catch (...) {}
        } else if(c == KEY_DOWN) {
            cursorToX(1);
            clrtoeol();
            hist.next();
            try {
                printw(hist.getCurrent().c_str());
                read = hist.getCurrent().size();
                strncpy(start, hist.getCurrent().c_str(), read);
                line = start + read;
            } catch (...) {}
        } else if (c == 8) {
            int y, x;
            getyx(stdscr, y, x);
            --x;
            if (x < 1)
                ++x;
            else {
                --read;
                --line;
            }
            move(y, x);
            delch();
        } else if (utilities::isAcceptable(c)) {
            printw("%c", c);
            *(line++) = c;
        }
        refresh();
        if ((c == '\n'))
            break;
    }
    *line = '\0';
    if (*start != '\0')
        hist.save(string(start));
    if (save)
        hist.write();
    nocbreak();
    halfdelay(3);
    if (read <= len)
        return (0);
    else
//        buffer too short
        return (-1);
}

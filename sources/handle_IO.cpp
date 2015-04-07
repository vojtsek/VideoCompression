#define _FILE_OFFSET_BITS 64
#include "headers/include_list.h"
#include "headers/handle_IO.h"
#include "headers/commands.h"
#include "headers/defines.h"

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

std::string loadInput(const std::string &histf, const std::string &msg,
                      bool save, bool changeable) {
    std::unique_lock<std::mutex> lck(DATA->m_data.I_mtx, std::defer_lock);
    std::unique_lock<std::mutex> lck2(DATA->m_data.O_mtx, std::defer_lock);

    // acquire locks for both input and output
    lck.lock();
    DATA->m_data.using_I = true;
    lck2.lock();
    // shows the msg and prompts for input
    DATA->m_data.using_O = true;
    cursToQuestion();
    printw("%s", msg.c_str());
    char line[LINE_LENGTH];
    curs_set(0);
    cursToCmd();
    // to print prompt
    cursorToX(0);
    printw(">");
    refresh();


    // load line
    getLine(line, LINE_LENGTH, histf, save, changeable);
    curs_set(0);
    // clears the lines
    cursorToX(0);
    clrtoeol();
    cursToQuestion();
    clrtoeol();
    refresh();
    // free the locks
    DATA->m_data.using_O = false;
    lck2.unlock();
    DATA->m_data.using_I = false;
    lck.unlock();
    DATA->m_data.IO_cond.notify_one();

    return std::string(line);
}

int64_t reportFileProgress(const std::string &file, long desired) {
    long fs = 0, old = 0;
    // how many times try it
    int64_t tries = 20;
    double percent;
    while (fs < desired) {
        // get the file size
        fs = OSHelper::getFileSize(file);
        if (fs < 0) {
            fs = 0;
        }
        // no change since last time
        if (fs == old) {
            // decrease tries - no change
            if (fs > 0) {
                --tries;
            }
            // give it chance to make some progress
            usleep(150000);
        } else { // refresh number of tries
            tries = 20;
        }
        // it propably hanged
        if (!tries) {
            return -1;
        }
        old = fs;
        // shows the progress bar
        percent = (double) fs / (double) desired;
        printProgress(percent);
        // wait for 10 ms
        usleep(10000);
    }

    return 0;
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

void reportDebug(const std::string &msg, int64_t lvl) {
    if (lvl <= DATA->config.debug_level) {
        DATA->io_data.status_handler.add(msg, DEBUG);
        DATA->io_data.status_handler.print();
    }
}

void initCurses() {
    initscr();
    // allows reading function keys
    keypad(stdscr, TRUE);
    // resizing
    signal (SIGWINCH, NULL);
    // implicitly not show pressed keys
    noecho();
    // input immediately available
    halfdelay(3);
    // start using colors
    start_color();
    // define colors
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
    // set background
    wbkgd(stdscr, COLOR_PAIR(BG));
}

void printProgress(double percent) {
    unique_lock<mutex> lck(DATA->m_data.O_mtx, defer_lock);
    lck.lock();
    DATA->m_data.using_O = true;
    // move cursor to the given place
    cursToPerc();
    clrtoeol();
    // prints number of percent done
    printw("(%lu%%)", (int) (percent * 100));
    attron(COLOR_PAIR(CYANALL));
    // shows the filebar
    for(int64_t i = 0; i < percent * (getmaxx(stdscr) - 7); ++i)
        printw("#");
    attroff(COLOR_PAIR(CYANALL));
    refresh();
    DATA->m_data.using_O = false;
    lck.unlock();
}

void cursToCmd() {
    int64_t max_x, max_y;
    // loads the max coordinates and oves the cursor
    getmaxyx(stdscr, max_y, max_x);
    move(max_y - 1, 1);
    clrtoeol();
}

void cursToInfo() {
    move(3, 0);
}

void cursToStatus() {
    if (!DATA->io_data.status_y) {
        DATA->io_data.status_y = getmaxy(stdscr) - 4;
    }
    static int64_t status_y = DATA->io_data.status_y;
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
    // move cursor to progress bar y coordinate and clear
    cursToPerc();
    clrtoeol();
}

void cursorToX(int64_t nx) {
    int64_t y, x;
    getyx(stdscr, y, x);
    move(y, nx);
}

void clearNlines(int64_t n) {
    int64_t orig_x, orig_y, x, y;
    getyx(stdscr, y, x);
    // remembers the original position
    orig_x = x;
    orig_y = y;
    // clears lines and moves to the original position
    while(n--) {
        move(y++, 0);
        clrtoeol();
    }
    move(orig_y, orig_x);
}

int64_t getLine(char *line, int64_t len,
                const std::string &histf, bool save, bool changeable) {
    HistoryStorage hist(histf);
    // buffer to store the input
    char *start = line;
    *line = '\0';
    nocbreak();
    // immediate input
    cbreak();
    wchar_t c;
    bool first = true;
    int64_t read = 0;
    while(++read <= len) {
        // if not changeable and first -> load the first option by
        // simulating key up
        if (!first || changeable) {
            c = getch();
        } else {
            c = KEY_UP;
        }
        first = false;
        // previous option
        if (c == KEY_UP) {
            cursorToX(2);
            clrtoeol();
            hist.prev();
            try {
                // loads the previous option, prints and copies into the buffer
                cursToCmd();
                printw(hist.getCurrent().c_str());
                read = hist.getCurrent().size();
                strncpy(start, hist.getCurrent().c_str(), read);
                line = start + read;
            } catch (...) {}
        // next option
        } else if(c == KEY_DOWN) {
            cursorToX(2);
            clrtoeol();
            hist.next();
            try {
                // loads the next option, prints and copies into the buffer
                cursToCmd();
                printw(hist.getCurrent().c_str());
                read = hist.getCurrent().size();
                strncpy(start, hist.getCurrent().c_str(), read);
                line = start + read;
            } catch (...) {}
        // backspace
        } else if ((c == 8) && changeable) {
            int64_t y, x;
            getyx(stdscr, y, x);
            // moves the x coordinate back
            // strips the buffer
            if (x > 1) {
                --x;
                --read;
                --line;
            }
            // moves the cursor back and deletes char
            move(y, x);
            delch();
        // other characters just adds to the buffer
        } else if (utilities::isAcceptable(c)) {
            printw("%c", c);
            *(line++) = c;
        }
        refresh();
        // return ends
        if ((c == '\n')) {
            break;
        }
    }
    // proper termination
    *line = '\0';
    // optionally saves the option
    if (*start != '\0') {
        hist.save(string(start));
    }
    if (save) {
        hist.write();
    }
    nocbreak();
    halfdelay(3);
    // success
    if (read <= len) {
        return (0);
    //    buffer too short
    } else {
        return (-1);
    }
}

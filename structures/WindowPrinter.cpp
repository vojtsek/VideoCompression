#include "headers/defines.h"
#include "headers/include_list.h"

#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>

//  no need to synchronize scrolling or clearing - use only for info

int64_t WindowPrinter::add(const std::string &str, MSG_T type) {
    int64_t retval;
    // acquire the mutex to prevent bad reporting
    DATA->m_data.report_mtx.lock();
    ofs << str << std::endl;
    ofs.flush();
    // determines, where to add the message
    if ((direction == DOWN) || (direction == STATIC)) {
        q.push_back(make_pair(str, type));
        retval = q.size() - 1;
    } else {
        q.push_front(make_pair(str, type));
        retval = 0;
    }
    DATA->m_data.report_mtx.unlock();
    return retval;
}

void WindowPrinter::changeWin(WINDOW *nwin) {
    win = nwin;
}

void WindowPrinter::scrollQueue(bool up) {
    if (up) {
        // handles index overflow
        if (idx + length < q.size()) {
            ++idx;
        }
    } else {
        if (idx > 0) {
            --idx;
        }
    }
}

void WindowPrinter::clear() {
    DATA->m_data.report_mtx.lock();
    q.clear();
    DATA->m_data.report_mtx.unlock();
}

void WindowPrinter::updateAt(int64_t idx, std::string value, MSG_T type) {
    q.at(idx) = std::make_pair<>(value, type);
    print();
}

void WindowPrinter::setLength(int64_t l) {
    this->length = l;
}

void WindowPrinter::changeLogLocation(std::string log_location) {
    ofs.close();
    ofs.open(log_location);
}

void WindowPrinter::print() {
    std::unique_lock<std::mutex> lck(DATA->m_data.O_mtx, std::defer_lock);

    lck.lock();
    while (DATA->m_data.using_O)
        DATA->m_data.IO_cond.wait(lck);
    DATA->m_data.using_O = true;
    DATA->m_data.report_mtx.lock();
    // color the background
    wbkgd(win, COLOR_PAIR(BG));
    wmove(win, 0, 0);
    werase(win);
    int64_t y, col;
    if (start == BOTTOM) {
        // start from the bottom
        y = length - 2;
    } else {
        // start fro the top
        y = 0;
    }
    // first flag
    bool first = true;
    std::deque<printable_pair_T>::iterator s, e;
    // s points to start of the showed part, e is break
    if (direction != DOWN) {
        s = q.begin() + idx;
    } else {
        s = q.size() > (unsigned) length ?
                    q.end() - length : q.begin();
        s -= idx;
    }
    e = q.end();
    int64_t i = length;
    // shows the length or until reaches the end
    while ((i-- > 0) && (s != e)) {
        // loads the data and adds to output
        auto a = *s++;
        // first item is bold
        if (first && bolded) {
            wattron(win, A_BOLD);
        }
        // picks color
        col = DEFAULT;
        if (a.second == ERROR) {
            col = RED;
        }
        if (a.second == SUCCESS) {
            col = GREEN;
        }
        if (a.second == DEBUG) {
            col = BLUE;
        }
        if (col != DEFAULT) {
            wattron(win, COLOR_PAIR(col));
        }
        // moves the index in right direction
        if (start == BOTTOM) {
            --y;
        } else {
            ++y;
        }
        // prints the line
        mvwprintw(win, y, 1, "%s\n", a.first.c_str());
        wrefresh(win);
        // handles the color and weight
        if (col != DEFAULT) {
            wattroff(win,	COLOR_PAIR(col));
        }
        if (first && bolded) {
            first = false;
            wattroff(win, A_BOLD);
        }
    }
    refresh();
    // prints the border
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(win);
    DATA->m_data.report_mtx.unlock();
    DATA->m_data.using_O = false;
    lck.unlock();
    DATA->m_data.IO_cond.notify_one();
}

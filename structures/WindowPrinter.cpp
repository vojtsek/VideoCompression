#include "headers/defines.h"
#include "headers/include_list.h"

#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>

//the indexing is not permanent
int32_t WindowPrinter::add(const std::string &str, MSG_T type) {
    int32_t retval;
    DATA->m_data.report_mtx.lock();
    ofs << str << std::endl;
    ofs.flush();
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
    q.clear();
}

void WindowPrinter::updateAt(int32_t idx, std::string value, MSG_T type) {
    q.at(idx) = std::make_pair<>(value, type);
    print();
}

void WindowPrinter::setLength(int32_t l) {
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
    wbkgd(win, COLOR_PAIR(BG));
    wmove(win, 0, 0);
    werase(win);
    int32_t y, col;
    if (start == BOTTOM)
        y = length - 2;
    else
        y = 0;
    bool first = true;
    std::deque<printable_pair_T>::iterator s, e;
    if (direction != DOWN) {
        s = q.begin() + idx;
    } else {
        s = q.size() > (unsigned) length ?
                    q.end() - length : q.begin();
        s -= idx;
    }
    e = q.end();
    int32_t i = length;
    while ((i-- > 0) && (s != e)) {
        auto a = *s++;
        if (first && bolded) {
            wattron(win, A_BOLD);
        }
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
        if (col != DEFAULT)
            wattron(win, COLOR_PAIR(col));
        if (start == BOTTOM)
            --y;
        else
            ++y;
        mvwprintw(win, y, 1, "%s\n", a.first.c_str());
        wrefresh(win);
        if (col != DEFAULT) {
            wattroff(win,	COLOR_PAIR(col));
        }
        if (first && bolded) {
            first = false;
            wattroff(win, A_BOLD);
        }
    }
    refresh();
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(win);
    DATA->m_data.report_mtx.unlock();
    DATA->m_data.using_O = false;
    lck.unlock();
    DATA->m_data.IO_cond.notify_one();
}

#ifndef WINDOWPRINTER_H
#define WINDOWPRINTER_H

#include "headers/enums_types.h"
#include <fstream>
#include <deque>
#include <curses.h>

/*!
 * \brief The WindowPrinter class
 * maintains windows that show some data
 * the content to be shown is stored in the queue
 * only part of it is shown at a time
 * stores the content in the log file
 */
class WindowPrinter {
     //! \brief win window to show
    WINDOW *win;
    //! the queue containing the contents to show
    std::deque<printable_pair_T> q;
    bool bolded;
    //! specify which part of the queue is shown
    int64_t idx;
    //! length of the queue
    uint64_t length;
    //! ofstream pointing to the log file
    std::ofstream ofs;
public:
    //! specifies direction of the output
    PRINTER_DIRECTION direction;
    PRINTER_START start;
    WindowPrinter(PRINTER_DIRECTION dir, bool b,
                  PRINTER_START st, std::string log_location):
        bolded(b), idx(0), length(0),
        ofs(log_location), direction(dir), start(st) {
        win = stdscr;
    }

    /*!
     * \brief changeWin changes the associated window
     * \param nwin pointer to the new window
     */
    void changeWin(WINDOW *nwin);

    /*!
     * \brief add adds new message to the queue
     * \param msg the message to add
     * \param type specifies the color of the message
     * \return index of the added message
     */
    int64_t add(const std::string &msg, MSG_T type);

    /*!
     * \brief updateAt updates the message on the specified index
     * \param idx index of the message
     * \param value new value
     * \param type new message type (i.e. color)
     */
    void updateAt(int64_t idx, std::string value, MSG_T type);

    /*!
     * \brief changeLogLocation specifies where to store the log
     * \param log_location new log location
     */
    void changeLogLocation(std::string log_location);

    /*!
     * \brief clear clears the content of the queue
     */
    void clear();

    /*!
     * \brief setLength sets the length of the part shown
     * \param l new length
     */
    void setLength(int64_t l);

    /*!
     * \brief scrollQueue moves the part of the queue that is shown
     * \param up specifies the direction of the scroll
     */
    void scrollQueue(bool up);

    /*!
     * \brief print shows the content
     */
    void print();
};

#endif // WINDOWPRINTER_H

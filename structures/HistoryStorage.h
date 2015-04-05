#ifndef HISTORYSTORAGE_H
#define HISTORYSTORAGE_H

/*!
 * \brief The HistoryStorage class
 * reads the history of inputted values from the text file
 * maintains the history, so user can traverse it
 */
class HistoryStorage {
    //! lines of the history
    std::vector<std::string> history;
    //! name of the text file
    std::string filename;
    //! current index
    uint64_t c_index;
public:
    /*!
     * \brief HistoryStorage constructor loads the history from the file
     * \param fn path to the file containing the history
     */
    HistoryStorage(const std::string &fn);

    /*!
     * \brief prev "Arrow UP", previous line, adjusts the index
     */
    void prev();

    /*!
     * \brief next "Arrow DOWN", next line, adjusts the index
     */
    void next();

    /*!
     * \brief save saves inputted line to the history, without writing it to the file
     * \param line new inputted line
     */
    void save(std::string line);

    /*!
     * \brief write writes the current history to the associated file
     */
    void write();
    std::string &getCurrent();
};


#endif // HISTORYSTORAGE_H

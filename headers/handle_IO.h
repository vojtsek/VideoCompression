#include <string>
#ifndef HADLEIO_H
#define HANDLEIO_H

/*!
 * \brief getLine gets one line of input from the user, handles history
 * \param line pointer to bufer
 * \param len buffer length
 * \param histf path to the file with history
 * \param save whether update history file
 * \param changeable if input is actually accepted or just selection of choices
 * \return zero on success
 * reads characters until new line
 * uses the file with history
 */
int64_t getLine(char *line, int64_t len,
                const std::string &histf, bool save, bool changeable);

/*!
 * \brief initCurses some curses initialization
 */
void initCurses();

/*!
 * \brief cursToInfo moves the cursor to the info y coordinate
 */
void cursToInfo();

/*!
 * \brief cursToQuestion moves the cursor to the progress bar  y coordinate
 */
void cursToPerc();

/*!
 * \brief cursToCmd moves the cursor to the  y coordinate for user input
 */
void cursToCmd();

/*!
 * \brief cursToStatus() moves the cursor to the status y coordinate
 */
void cursToStatus();

/*!
 * \brief cursToQuestion moves the cursor to the question y coordinate
 */
void cursToQuestion();

/*!
 * \brief clearNlines clears given number of lines
 * \param n number of lines to clear
 * The given number of lines from current position is cleared,
 * the cursor is eventually at the same place as in the start
 */
void clearNlines(int64_t n);

/*!
 * \brief clearProgress clears the progress bar
 */
void clearProgress();

/*!
 * \brief reportFileProgress reflects the changes in filesize
 * \param file path to the file being watched
 * \param desired final filesize
 * \return zero on success
 * Periodically checks the filesize, computes the ratio
 * and shoes the progress bar
 */
int64_t reportFileProgress(const std::string &file, long desired);

/*!
 * \brief cursorToX moves the cursor to specified x coordinate
 * \param nx desired x coordinate
 */
void cursorToX(int64_t nx);

/*!
 * \brief reportError reports the error message
 * \param err message to report
 * Reports the message using the WindowPrinter instance
 * which is handling status window;
 * Message is red.
 */
void reportError(const std::string &err);

/*!
 * \brief reportSuccess reports the success message
 * \param msg message to report
 * Reports the message using the WindowPrinter instance
 * which is handling status window;
 * Message is green.
 */
void reportSuccess(const std::string &msg);

/*!
 * \brief reportStatus reports the message
 * \param msg message to report
 * Reports the message using the WindowPrinter instance
 * which is handling status window;
 * Message is plain.
 */
void reportStatus(const std::string &msg);

/*!
 * \brief reportDebug reports the debug message
 * \param msg message to report
 * \param lvl importance level of the message
 * Reports the message using the WindowPrinter instance
 * which is handling status window,
 * iff the current level of debugging is lesser or equal to the given one.
 * Message is blue.
 */
void reportDebug(const std::string &msg, int64_t lvl);

/*!
 * \brief printProgress prints the progress bar
 * \param percent how long it should be
 * Prints line of characters, its length is determined by the parameter
 */
void printProgress(double percent);

/*!
 * \brief loadInput loads line of user input
 * \param histf text file with history
 * \param msg message to prompt
 * \param save whether save to history
 * \param changeable if the input should be editable or just choosen with arrow keys
 * \return string with inputted line
 * Prompts the user for input,
 * handles the input, providing history (using getLine())
 */
std::string loadInput(const std::string &histf,
                      const std::string &msg, bool save, bool changeable);

#endif

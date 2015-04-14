#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>

#include "headers/commands.h"
#include "structures/NetworkHandler.h"
#include "headers/handle_IO.h"
#include "headers/defines.h"

#ifndef UTILS_H
#define UTILS_H
class Command;
class TransferInfo;
namespace utilities
{

    int64_t readCmd(std::stringstream &ins, cmd_storage_t &cmds, VideoState &st);

    /*!
     * \brief acceptCmd reacts to user command input
     * \param cmds mapping of commands from keys to structures
     * \return zero on success
     * Non blockingly waits for input, reads one key
     * and tries to map it to a command,
     * spawns the command execution in separate thread and returns.
     *
     */
    int64_t acceptCmd(cmd_storage_t &cmds);

    /*!
     * \brief computeDuration computes the difference between two string timestamps
     * \param t1 timestamp1
     * \param t2 timestamp2
     * \return numeric difference of the timestamps
     */
    int64_t computeDuration(std::string t1, std::string t2);

    /*!
     * \brief extract extracts text from string from the given word
     * \param text text from which should extract
     * \param from words to extract from
     * \param count how many words should extract
     * \return vector of extracted values
     */
    std::vector<std::string> extract(const std::string text, const std::string from, int64_t count);

    /*!
     * \brief split splits the given into parts
     * \param content string to split
     * \param sep separator
     * \return vector of parts between the occurences of the separator
     */
    std::vector<std::string> split(const std::string &content, char sep);

    /**
     * \brief utilities::getTimestamp gets string representation of the current time
     * \return current time since epoch start, in MILIseconds
     */
    std::string getTimestamp();

    /*!
     * \brief m_itoa converts the integer to string
     * \param n integer to convert
     * \return string representation of the integer
     */
    std::string m_itoa(int64_t n);

    /*!
     * \brief formatString justifies the line, supposed to be field name and value
     * \param str1 name of the field
     * \param str2 value
     * \return justified string
     */
    std::string formatString(std::string str1, std::string str2);

    /*!
     * \brief pathFromChunk creates path to file associated with the structure
     * \param ti structure to obtain path from
     * \param which what type of path
     * \return created path
     */
    std::string pathFromChunk(TransferInfo *ti,
                              const std::string &which);
    /*!
     * \brief listCmds print the list of available commands
     */
    void listCmds();

    /*!
     * \brief printOverallState prints information about encoding process
     * \param state pointer to VideoState instance
     *
     */
    void printOverallState(VideoState *state);

    /*!
     * \brief knownCodec checks, whether the codec is known
     * \param cod string representation of the codec
     * \return true if codec is known
     */
    bool knownCodec(const std::string &cod);

    /*!
     * \brief knownFormat  checks, whether the given format (i.e. extension) is known
     * \param format extension to check
     * \return true if the format is known
     */
    bool knownFormat(const std::string &format);

    /*!
     * \brief knownQuality  checks, whether the choosen quality is valid
     * \param quality given quality
     * \return true if the quality is known
     */
    bool knownQuality(const std::string &quality);

    /*!
     * \brief isAcceptable checks if the character is valid input
     * \param c the character to check
     * \return true if the character is acceptable
     */
    bool isAcceptable(char c);
}
#endif

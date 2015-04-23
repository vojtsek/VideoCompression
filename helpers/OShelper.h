#ifndef OSHelper_HELPER_H
#define OSHelper_HELPER_H

/*!
 * contains multiple functions to help interact with the os
 */
namespace OSHelper {

/*!
 * \brief OSHelper::checkFile
 * \param path - path to the file
 * \return 0 in case of success
 * checks, whether the file is known video file
 */
    int64_t checkFile(std::string &path);

/*!
 * \brief OSHelper::rmrDir remove directory
 * \param dir path to the directory
 * \param recursive whether delete content recursively
 * \return  0 in case of success
 * removes content of the directory and the directory itself
 * recursive deletion need to be specified
 */
    int64_t rmrDir(std::string dir, bool rec);

/*!
 * \brief OSHelper::prepareDir ensures the given directory exists
 * \param location location to the desired directory
 * \param destroy whether delete contents in case of existence
 * \return 0 on success
 * creates the desired directory, and others in path if needed
 * existing directory is treated according to the destroy parameter
 */
    int64_t prepareDir(std::string loc, bool destroy);


/*!
 * \brief OSHelper::mkDir creates directory
 * \param location path to the directory
 * \param destroy whether remove existing
 * \return 0 on success
 */
    int64_t mkDir(std::string dir, bool rec);

/*!
 * \brief OSHelper::rmFile removes file
 * \param fp path to the file
 * \return 0 on success
 */
    int64_t rmFile(std::string fp);

/*!
 * \brief OSHelper::runExternal spawns external command
 * \param stdo string to save stdout
 * \param stde string to save stderr
 * \param limit how many seconds before the process is killed
 * \param cmd name of the command to spawn
 * \param numargs number of parameters
 * \return 0 on success
 * spawns the xternal command, parameters are given optionally
 * uses fork & exec, waits for the result_type
 * stderr and stdout are saved in the corresponding strings
 */
    int64_t runExternal(
            std::string &o, std::string &e, int64_t limit,
            const char *cmd, int64_t numargs, ...);

    /*!
         * \brief loadFile loads the given file into VideoState
         * \param fpath path to the file to load
         * \param state pointer to the VideoState structure
         * \return zero on success
         */
        int64_t loadFile(const std::string fpath, VideoState *state);
 /*!
 * \brief OSHelper::getFileSize
 * \param file path to the file
 * \return size of the file in bytes, -1 in case of failure
 */
    int64_t getFileSize(const std::string &file);

 /*!
 * \brief OSHelper::getExtension extracts file extension
 * \param str name of the file
 * \return file extension
 */
    std::string getExtension(const std::string &str);

 /*!
 * \brief OSHelper::getBasename extracts the file's basename
 * \param str path to the files
 * \return basename of the file, without extension
 */
    std::string getBasename(const std::string &str);

/*!
 * \brief OSHelper::isFileOk checks, whether the file exists and is nonempty
 * \param fp
 * \return true if the file is ok
 */
    bool isFileOk(const std::string &fp);

}
#endif // OSHelper_HELPER_H

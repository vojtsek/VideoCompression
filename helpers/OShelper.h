#ifndef OSHelper_HELPER_H
#define OSHelper_HELPER_H

/*!
 * contains multiple functions to help interact with the os
 */
namespace OSHelper {
    int32_t checkFile(std::string &path);
    int32_t prepareDir(std::string loc, bool destroy);
    int32_t mkDir(std::string dir, bool rec);
    int32_t rmrDir(std::string dir, bool rec);
    int32_t rmFile(std::string fp);
    int32_t runExternal(std::string &o, std::string &e, char *cmd, int32_t numargs, ...);
    int64_t getFileSize(const std::string &file);
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    bool isFileOk(const std::string &fp);
}

#endif // OSHelper_HELPER_H

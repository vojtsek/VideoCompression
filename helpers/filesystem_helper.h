#ifndef FILESYSTEM_HELPER_H
#define FILESYSTEM_HELPER_H

namespace utilities {
    int32_t checkFile(std::string &path);
    int32_t prepareDir(std::string loc, bool destroy);
    int32_t mkDir(std::string dir, bool rec);
    int32_t rmrDir(std::string dir, bool rec);
    int32_t rmFile(std::string fp);
    int32_t runExternal(std::string &o, std::string &e, char *cmd, int32_t numargs, ...);
    long getFileSize(const std::string &file);
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    bool isFileOk(std::string fp);
}

#endif // FILESYSTEM_HELPER_H

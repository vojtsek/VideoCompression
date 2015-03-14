#ifndef FILESYSTEM_HELPER_H
#define FILESYSTEM_HELPER_H

namespace utilities {
    int checkFile(std::string &path);
    int prepareDir(std::string loc, bool destroy);
    int mkDir(std::string dir, bool rec);
    int rmrDir(std::string dir, bool rec);
    int rmFile(std::string fp);
    int runExternal(std::string &o, std::string &e, char *cmd, int numargs, ...);
    long getFileSize(const std::string &file);
    std::string getExtension(const std::string &str);
    std::string getBasename(const std::string &str);
    bool isFileOk(std::string fp);
}

#endif // FILESYSTEM_HELPER_H

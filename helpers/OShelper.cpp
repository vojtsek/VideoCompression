#include "headers/include_list.h"

#include <sys/stat.h>
#include <wait.h>
#include <dirent.h>
#include <string.h>


/*!
 * \brief OSHelper::checkFile
 * \param path - path to the file
 * \return 0 in case of success
 * checks, whether the file is known video file
 */
int32_t OSHelper::checkFile(std::string &path) {
    struct stat info;

    if (stat (path.c_str(), &info) == -1){
        reportError("An error occured while controlling the file: " + path);
        return (-1);
    }
    std::string extension = getExtension(path);
    if (extension.empty()) {
        reportError("Invalid file extension.");
        return (-1);
    }
    std::vector<std::string> video_ext = DATA->getKnownFormats();
    if (find(video_ext.begin(), video_ext.end(), extension) == video_ext.end()) {
        reportError("Unknown file extension");
        return (-1);
    }
    return (0);
}

/*!
 * \brief OSHelper::rmrDir remove directory
 * \param dir path to the directory
 * \param recursive whether delete content recursively
 * \return  0 in case of success
 * removes content of the directory and the directory itself
 * recursive deletion need to be specified
 */
int32_t OSHelper::rmrDir(std::string dir, bool recursive) {
  DIR *d, *t;
  struct dirent *entry;
  char abs_fn[256];
  d = opendir(dir.c_str());
  if (d == NULL)
    return (-1);
  while ((entry = readdir(d))) {
    if ((strcmp(entry->d_name, ".") == 0) ||
      (strcmp(entry->d_name, "..") == 0))
      continue;
    sprintf(abs_fn, "%s/%s", dir.c_str(), entry->d_name);
    if ((t = opendir(abs_fn))) {
      closedir(t);
      if(recursive)
        rmrDir(abs_fn, true);
      else
         return (-1);
    } else {
      if (unlink(abs_fn) == -1)
          return (-1);
    }
  }
  closedir(d);
  rmdir(dir.c_str());
  return (0);
}

/*!
 * \brief OSHelper::rmFile removes file
 * \param fp path to the file
 * \return 0 on success
 */
int32_t OSHelper::rmFile(std::string fp) {
    if (unlink(fp.c_str()) == -1) {
        reportDebug(fp + ": Failed to remove.", 1);
        return -1;
    }
    return 0;
}

/*!
 * \brief OSHelper::prepareDir ensures the given directory exists
 * \param location location to the desired directory
 * \param destroy whether delete contents in case of existence
 * \return 0 on success
 * creates the desired directory, and others in path if needed
 * existing directory is treated according to the destroy parameter
 */
int32_t OSHelper::prepareDir(std::string location, bool destroy) {
    std::string directory, current;
    location = location.substr(
                DATA->config.getStringValue("WD").size() + 1, location.length());
    uint32_t pos;
    current = DATA->config.getStringValue("WD");
    while ((pos = location.find('/')) != std::string::npos) {
        directory = location.substr(0, pos);
        if ( location.find('/') != std::string::npos) {
            location = location.substr(pos + 1, location.length());
        }
        current += "/" + directory;
        //todo remove rest?
        if (OSHelper::mkDir(current, destroy) == -1) {
            return -1;
        }
    }
    current += "/" + location;
    return OSHelper::mkDir(current, destroy);
}

/*!
 * \brief OSHelper::mkDir creates directory
 * \param location path to the directory
 * \param destroy whether remove existing
 * \return 0 on success
 */

int32_t OSHelper::mkDir(std::string location, bool destroy) {
    if (mkdir(location.c_str(), 0700) == -1) {
        switch (errno) {
        case EEXIST:
            if (destroy) {
                if (rmrDir(location.c_str(), false) == -1)
                    return -1;
                if (mkdir(location.c_str(), 0700) == -1)
                    return -1;
            }
            break;
        default:
            return -1;
            break;
        }
    }
    return 0;
}

/*!
 * \brief OSHelper::runExternal spawns external command
 * \param stdo string to save stdout
 * \param stde string to save stderr
 * \param cmd name of the command to spawn
 * \param numargs number of parameters
 * \return 0 on success
 * spawns the xternal command, parameters are given optionally
 * uses fork & exec, waits for the result_type
 * stderr and stdout are saved in the corresponding strings
 */
int32_t OSHelper::runExternal(std::string &stdo, std::string &stde, char *cmd, int32_t numargs, ...) {
    pid_t pid;
    int32_t pd_o[2], pd_e[2], j;
    size_t bufsize = 65536;
    std::string whole_command(cmd);
    char buf_o[bufsize], buf_e[bufsize];
    char *bo = buf_o, *be = buf_e;
    pipe(pd_o);
    pipe(pd_e);
    switch (pid = fork()) {
    case 0: {
        va_list arg_ptr;
        va_start (arg_ptr, numargs);
        char *args[numargs + 3];
        // TODO: safety!
        for(j = 0 ; j < numargs; ++j) {
            char *arg = va_arg(arg_ptr, char *);
            args[j] = arg;
            whole_command += " ";
            whole_command += arg;
        }
        reportDebug("Spawning '" + whole_command + "'", 1);
        args[j] = nullptr;
        va_end(arg_ptr);
        close(STDOUT_FILENO);
        close(pd_o[0]);
        dup(pd_o[1]);
        close(STDERR_FILENO);
        close(pd_e[0]);
        dup(pd_e[1]);
        char command[BUF_LENGTH];
        strcpy(command, cmd);
        if ((execvp(command, args)) == -1) {
            reportError("Error while spawning external command.");
            return (-1);
        }
        break;
        }
    case -1:
        close(pd_o[0]);
        close(pd_o[1]);
        close(pd_e[0]);
        close(pd_e[1]);
        return (-1);
        break;
    default:
        close(pd_o[1]);
        close(pd_e[1]);
        stdo = stde = "";
        int32_t st;
        wait(&st);
        while(read(pd_o[0], bo, 1) == 1){
            bo++;
            //always positive anyway
            if((unsigned)(bo - buf_o) > bufsize){
                *bo = '\0';
                stdo += std::string(buf_o);
                bo = buf_o;
            }
        }
        while(read(pd_e[0], be, 1) == 1){
            be++;
            if((unsigned)(be - buf_e) > bufsize){
                *be = '\0';
                stde += std::string(buf_e);
                be = buf_e;
            }
        }
        stdo += std::string(buf_o);
        stde += std::string(buf_e);
        close(pd_o[0]);
        close(pd_e[0]);
        break;
    }
    return 0;
}

/*!
 * \brief OSHelper::isFileOk checks, whether the file exists and is nonempty
 * \param fp
 * \return true if the file is ok
 */
bool OSHelper::isFileOk(const std::string &fp) {
    struct stat st;
    if (stat(fp.c_str(), &st) == -1) {
        return false;
    }
    return true;
}

/*!
 * \brief OSHelper::getFileSize
 * \param file path to the file
 * \return size of the file in bytes
 */
int64_t OSHelper::getFileSize(const std::string &file) {
    struct stat64 finfo;
    if (lstat64(file.c_str(), &finfo) == -1)
        return (-1);
    return finfo.st_size;
}

/*!
 * \brief OSHelper::getExtension extracts file extension
 * \param str name of the file
 * \return file extension
 */
std::string OSHelper::getExtension(const std::string &str) {
    size_t pos = str.rfind('.');
    std::string extension("");
    if (pos != std::string::npos)
        extension = str.substr(pos + 1);
    return extension;
}

/*!
 * \brief OSHelper::getBasename extracts the file's basename
 * \param str path to the files
 * \return basename of the file, without extension
 */
std::string OSHelper::getBasename(const std::string &str) {
    size_t pos = str.rfind('/');
    std::string basename("");
    if (pos == std::string::npos)
        return std::string("");
    basename = str.substr(pos + 1);
    pos = basename.rfind('.');
    if (pos == std::string::npos)
        return basename;
    basename = basename.substr(0, pos);
    return basename;
}

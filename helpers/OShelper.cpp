#include "headers/include_list.h"

#include <sys/stat.h>
#include <wait.h>
#include <dirent.h>
#include <string.h>


int64_t OSHelper::checkFile(std::string &path) {
    struct stat info;

    if (stat (path.c_str(), &info) == -1){
        reportError("An error occured while controlling the file: " + path);
        return (-1);
    }
    // part after the last dot
    std::string extension = getExtension(path);
    if (extension.empty()) {
        reportError("Invalid file extension.");
        return (-1);
    }
    // checks whether supported
    std::vector<std::string> video_ext = DATA->getKnownFormats();
    if (find(video_ext.begin(), video_ext.end(), extension) == video_ext.end()) {
        reportError("Unknown file extension");
        return (-1);
    }
    return (0);
}

int64_t OSHelper::rmrDir(std::string dir, bool recursive) {
  DIR *d, *t;
  struct dirent *entry;
  char abs_fn[256];
  d = opendir(dir.c_str());
  if (d == NULL)
    return (-1);
  // loops through all entries in the directory
  while ((entry = readdir(d))) {
      // ignoring special cases
    if ((strcmp(entry->d_name, ".") == 0) ||
      (strcmp(entry->d_name, "..") == 0))
      continue;
    sprintf(abs_fn, "%s/%s", dir.c_str(), entry->d_name);

    // if the entry is a directory
    if ((t = opendir(abs_fn))) {
      closedir(t);
      // if deletion should dive recursively
      if (recursive) {
        rmrDir(abs_fn, true);
      } else {
         return -1;
      }

     // entry is not a directory
    } else {
        // remove the file
      if (unlink(abs_fn) == -1) {
          return -1;
      }
    }
  }
  closedir(d);
  // finally removes the directory itself
  rmdir(dir.c_str());
  return (0);
}

int64_t OSHelper::rmFile(std::string fp) {
    // file does not exist - nothing to remove
    if (!OSHelper::isFileOk(fp)) {
        return 0;
    }
    if (unlink(fp.c_str()) == -1) {
        return -1;
    }
    return 0;
}

int64_t OSHelper::prepareDir(std::string location, bool destroy) {
    std::string directory, current;
    // the WD should definitely exist already
    location = location.substr(
                DATA->config.getStringValue("WD").size() + 1,
                location.length());
    uint64_t pos;
    // holds the current position
    current = DATA->config.getStringValue("WD");
    // further directories on the path
    while ((pos = location.find('/')) != std::string::npos) {
        // extract next directory
        directory = location.substr(0, pos);
        // shorten the location - holds the rest og the path
        if ( location.find('/') != std::string::npos) {
            location = location.substr(pos + 1, location.length());
        }
        // update current
        current += "/" + directory;
        // creates the directory
        if (OSHelper::mkDir(current, destroy) == -1) {
            return -1;
        }
    }
    // eventually the location contains the last directory
    current += "/" + location;
    return OSHelper::mkDir(current, destroy);
}

int64_t OSHelper::mkDir(std::string location, bool destroy) {
    if (mkdir(location.c_str(), 0700) == -1) {
        if (errno == EEXIST) {
        // the directory existed already
            // if destroy, the existing direcotry is removed recursively
            // end with error otherwise
            if (destroy) {
                if (rmrDir(location.c_str(), false) == -1) {
                    return -1;
                }
                if (mkdir(location.c_str(), 0700) == -1) {
                    return -1;
                }
            }
        } else {
            return -1;
        }
    }
    return 0;
}

int64_t OSHelper::runExternal(std::string &stdo, std::string &stde, const char *cmd, int64_t numargs, ...) {
    pid_t pid;
    int pd_o[2], pd_e[2], j;
    size_t bufsize = 65536;
    std::string whole_command(cmd);
    char buf_o[bufsize], buf_e[bufsize];
    char *bo = buf_o, *be = buf_e;
    // creates pipes
    pipe(pd_o);
    pipe(pd_e);
    // forks
    switch (pid = fork()) {
    // child process
    case 0: {
        va_list arg_ptr;
        va_start (arg_ptr, numargs);
        // obtain parameters
        char *args[numargs + 3];
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
        // execute the desired programme
        if ((execvp(command, args)) == -1) {
            reportError("Error while spawning external command.");
            return -1;
        }
        break;
        }
        // error while forking
    case -1:
        close(pd_o[0]);
        close(pd_o[1]);
        close(pd_e[0]);
        close(pd_e[1]);
        return (-1);
        break;
        // parent
    default:
        close(pd_o[1]);
        close(pd_e[1]);
        stdo = stde = "";
        int64_t st;
        // wait for the child to end
        wait(&st);
        // reads the output from the pipe to the buffer
        // TODO: should not go output to the file?
        while(read(pd_o[0], bo, 1) == 1){
            bo++;
            //always positive anyway
            if((unsigned)(bo - buf_o) > bufsize){
                *bo = '\0';
                stdo += std::string(buf_o);
                bo = buf_o;
            }
        }
        // same with stderr
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

bool OSHelper::isFileOk(const std::string &fp) {
    struct stat64 st;
    // tries to stat the file
    if (stat64(fp.c_str(), &st) == -1) {
        return false;
    }
    return true;
}

int64_t OSHelper::getFileSize(const std::string &file) {
    struct stat64 finfo;
    // TODO: really 64?
    // negative RV indicates error
    if (lstat64(file.c_str(), &finfo) == -1)
        return -1;
    return finfo.st_size;
}

std::string OSHelper::getExtension(const std::string &str) {
    size_t pos = str.rfind('.');
    std::string extension("");
    // extracts the part behind last period
    if (pos != std::string::npos) {
        extension = str.substr(pos + 1);
    }
    return extension;
}

std::string OSHelper::getBasename(const std::string &str) {
    size_t pos = str.rfind('/');
    std::string basename("");
    // the file itself is basename
    if (pos == std::string::npos) {
        basename = str;
    } else {
        basename = str.substr(pos + 1);
    }
    pos = basename.rfind('.');
    // strips the extension, if any
    if (pos == std::string::npos) {
        return basename;
    }
    basename = basename.substr(0, pos);
    return basename;
}

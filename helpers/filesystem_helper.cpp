#include "headers/include_list.h"

#include <sys/stat.h>
#include <wait.h>
#include <dirent.h>
#include <string.h>

int utilities::checkFile(std::string &path) {
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

int utilities::rmrDir(std::string dir, bool recursive) {
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

int utilities::rmFile(std::string fp) {
    if (unlink(fp.c_str()) == -1) {
        reportDebug(fp + ": Failed to remove.", 1);
        return -1;
    }
    return 0;
}

int utilities::prepareDir(std::string location, bool destroy) {
    std::string directory, current;
    location = location.substr(
                std::string(WD).size() + 1, location.length());
    uint32_t pos;
    current = WD;
    while ((pos = location.find('/')) != std::string::npos) {
        directory = location.substr(0, pos);
        if ( location.find('/') != std::string::npos) {
            location = location.substr(pos + 1, location.length());
        }
        current += "/" + directory;
        //todo remove rest?
        if (utilities::mkDir(current, destroy) == -1) {
            return -1;
        }
    }
    current += "/" + location;
    return utilities::mkDir(current, destroy);
}

int utilities::mkDir(std::string location, bool destroy) {
    if (mkdir(location.c_str(), 0700) == -1) {
        switch (errno) {
        case EEXIST:
            if (destroy) {
                if (rmrDir(location.c_str(), false) == -1)
                    return (-1);
                if (mkdir(location.c_str(), 0700) == -1)
                    return (-1);
            }
            break;
        default:
            return (-1);
            break;
        }
    }
    return (0);
}

int utilities::runExternal(std::string &stdo, std::string &stde, char *cmd, int numargs, ...) {
    pid_t pid;
    int pd_o[2], pd_e[2], j;
    size_t bufsize = 65536;
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
        }
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
        int st;
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

bool utilities::isFileOk(std::string fp) {
    struct stat st;
    if (stat(fp.c_str(), &st) == -1) {
        return false;
    }
    return true;
}

long utilities::getFileSize(const std::string &file) {
    struct stat64 finfo;
    if (lstat64(file.c_str(), &finfo) == -1)
        return (-1);
    return finfo.st_size;
}

std::string utilities::getExtension(const std::string &str) {
    size_t pos = str.rfind('.');
    std::string extension("");
    if (pos != std::string::npos)
        extension = str.substr(pos + 1);
    return extension;
}

std::string utilities::getBasename(const std::string &str) {
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

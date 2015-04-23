#include "headers/include_list.h"
#include "structures/singletons.h"
#include "headers/defines.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

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
    std::string cwd = DATA->config.getStringValue("WD").substr(
                0, DATA->config.getStringValue("WD").rfind('/')
                );
    location = location.substr(
                cwd.size() + 1,
                location.length());
    uint64_t pos;
    // holds the current position
    current = cwd;
    // further directories on the path
    while ((pos = location.find('/')) != std::string::npos) {
        // extract next directory
        directory = location.substr(0, pos);
        // shorten the location - holds the rest og the path
        if ( location.find('/') != std::string::npos) {
            location = location.substr(pos + 1, location.length());
        }
        // update current
        current += PATH_SEPARATOR + directory;
        // creates the directory
        if (OSHelper::mkDir(current, destroy) == -1) {
            return -1;
        }
    }
    // eventually the location contains the last directory
    current += PATH_SEPARATOR + location;
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

int64_t OSHelper::runExternal(
        std::string &stdo, std::string &stde, int64_t limit,
        const char *cmd, int64_t numargs, ...) {
    pid_t pid;
    int pd_o[2], pd_e[2], j;
    size_t bufsize = 65536;
    std::string whole_command(cmd);
    char buf_o[bufsize], buf_e[bufsize];
    char *bo = buf_o, *be = buf_e;
    // creates pipes
    pipe(pd_o);
    pipe(pd_e);

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
        reportDebug("Spawning '" + whole_command + "'", 2);
        args[j] = nullptr;
        va_end(arg_ptr);

    // forks
    switch (pid = fork()) {
    // child process
    case 0: {
        alarm(limit);
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
        siginfo_t infop;
        // wait for the child to end
        if (waitid(P_PID,
                   pid, &infop, WEXITED) == -1) {
            reportError("FAIL");
            return -1;
        }
        if (infop.si_code == CLD_KILLED) {
            reportError("KILLED");
            return -1;
        }
        if (infop.si_status != 0) {
            reportError("FAIL");
            return -1;
        }
        // reads the output from the pipe to the buffer
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


        std::ofstream ffofs;
        ffofs.open(LOG_PATH + PATH_SEPARATOR + "cmds.out", std::ios_base::app);
        ffofs << "-----------------------------------------------------------------" << std::endl;
        ffofs << whole_command << std::endl << std::endl;
        ffofs << "STDOUT:" << std::endl;
        ffofs << stdo << std::endl << std::endl;
        ffofs << "STDERR:" << std::endl;
        ffofs << stde << std::endl;
        ffofs << "-----------------------------------------------------------------" << std::endl;
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

int64_t OSHelper::loadFile(
        const std::string fpath, VideoState *state) {
    FileInfo &finfo = state->finfo;
    std::string path(fpath), out, err, err_msg("Error loading the file: ");
    rapidjson::Document document;
    std::stringstream ssd;
        // is file ok?
    try {
    if (OSHelper::checkFile(path) == -1){
        reportError("Loading the file " + path + " failed");
        throw 1;
    }
    // saves information in the FileInfo structure
    finfo.fpath = path;
    finfo.extension = "." + OSHelper::getExtension(path);
    finfo.basename = OSHelper::getBasename(path);
    // gain some info about the video
    if (OSHelper::runExternal(out, err, 510,
                               DATA->config.getStringValue("FFPROBE_LOCATION").c_str(), 6,
                               DATA->config.getStringValue("FFPROBE_LOCATION").c_str(),
                               finfo.fpath.c_str(), "-show_streams", "-show_format",
                              "-print_format", "json") < 0) {
        reportError("Error while getting video information.");
        throw 1;
    }
    if (err.find("Invalid data") != std::string::npos) {
        reportError("Invalid video file");
        state->resetFileInfo();
        throw 1;
    }

    // parse the JSON output and save
    if(document.Parse(out.c_str()).HasParseError()) {
        reportError(err_msg + "Parse error");
        throw 1;
    }
    if (!document.HasMember("format")) {
        reportError(err_msg);
        throw 1;
    }
    if(!document["format"].HasMember("bit_rate")) {
        reportError(err_msg);
        throw 1;
    }
    ssd.clear();
    ssd.str(document["format"]["bit_rate"].GetString());
    ssd >> finfo.bitrate;
    finfo.bitrate /= 8;

    if(!document["format"].HasMember("duration")) {
        reportError(err_msg);
        throw 1;
    }
    ssd.clear();
    ssd.str(document["format"]["duration"].GetString());
    ssd >> finfo.duration;

    if(!document["format"].HasMember("size")) {
        reportError(err_msg);
        throw 1;
    }
    ssd.clear();
    ssd.str(document["format"]["size"].GetString());
    ssd >> finfo.fsize;

    if(!document["format"].HasMember("format_name")) {
        reportError(err_msg);
        throw 1;
    }
    finfo.format = document["format"]["format_name"].GetString();

    if(!document.HasMember("streams")) {
        reportError(err_msg);
        throw 1;
    }
    const rapidjson::Value &streams = document["streams"];
    bool found = false;
    if (!streams.IsArray()) {
        reportError("Invalid video file");
        throw 1;
    }
    // codec information
    for(rapidjson::SizeType i = 0; i < streams.Size(); ++i) {
        if (streams[i].HasMember("codec_type") && (streams[i]["codec_type"].GetString() == std::string("video"))) {
            finfo.codec = streams[i]["codec_name"].GetString();
            found = true;
            break;
        }
    }
    if (!found) {
        reportError("Invalid video file");
        throw 1;
    }
        } catch (int) {
        return -1;
        }
    // loads the file to the VideoState
    state->loadFileInfo(finfo);
    reportSuccess(finfo.fpath + " loaded.");
    state->printVideoState();
    return 0;
}

#include "headers/include_list.h"
#include "structures/singletons.h"
#include "headers/defines.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filestream.h"

using namespace rapidjson;
using namespace std;

void CmdHelp::execute() {
}

void CmdDef::execute() {
    utilities::printOverallState(state);
}

void CmdScrollDown::execute() {
    DATA->io_data.info_handler.scrollQueue(true);
    DATA->io_data.info_handler.print();
}

void CmdScrollUp::execute() {
    DATA->io_data.info_handler.scrollQueue(false);
    DATA->io_data.info_handler.print();
}

void CmdAbort::execute() {
    if (DATA->state.working) {
            reportError("Aborting...");
            state->abort();
    } else {
        reportStatus("No process to abort.");
    }
}

bool CmdShow::execute(int64_t, sockaddr_storage &, void *) {
    return true;
}

void CmdShow::execute() {
    std::string what = loadInput("show.hist", "", false, false);
    // showing state
    if (what == "state") {
        utilities::printOverallState(state);
        // showing neighbors
    } else if (what.find("neighbors") != std::string::npos) {
        DATA->neighbors.printNeighborsInfo();
        // showing file info
    } else if (what.find( "file") != std::string::npos){
        if (state->finfo.fpath.empty()) {
            reportError("Please load the file first.");
            return;
        }
        state->printVideoState();
    }
}

void CmdStart::execute() {
    if (state->finfo.fpath.empty()) {
        reportError("Please load the file first.");
        return;
    }

    // is it not empty existing file?
    if (OSHelper::checkFile(state->finfo.fpath) == -1) {
        reportError("Invalid file.");
        return;
    }

    // Some job is currently being done
    if (DATA->state.working) {
        reportError("Already in working process.");
        return;
    }
    // print state and begin to split
    state->printVideoState();
    if (state->split() == -1) {
        reportError("Error while splitting the video file.");
        OSHelper::rmrDir(state->dir_location.c_str(), false);
        return;
    }
}

void CmdSetCodec::execute() {
    // reads the input and sets the codec
    std::string in = loadInput("codecs", "Enter desired output codec:", false, false);
    if (utilities::knownCodec(in)) {
        state->o_codec = in;
        reportSuccess("Output codec set to: " + in);
    } else {
        reportError(in + ": Unknown codec.");
        std::string msg;
        msg += "Available codecs: ";
        for (string c : Data::getKnownCodecs())
            msg += c += ", ";
        reportStatus(msg.substr(0, msg.length() - 2));
    }
}

void CmdSetChSize::execute() {
    //reads new chunk size
    std::string in = loadInput("", "Enter new chunk size (kB):", false, false);
    size_t nsize = DATA->config.getIntValue("CHUNK_SIZE");
    std::stringstream ss(in), msg;
    ss >> nsize;
    // is supposed to be stored in bytes
    state->changeChunkSize(nsize * 1024);
    msg << "Chunk size set to: " << nsize << " kB";
    reportSuccess(msg.str());
}

void CmdSetFormat::execute() {
    // loads the desired container format
    std::string in = loadInput("", "Enter desired output format:", false, false);
    if (utilities::knownFormat(in)) {
        state->o_format = "." + in;
        reportSuccess("Output format set to: " + in);
    } else {
        reportError(in + ": Not a valid format.");
        std::string msg;
        msg += "Available formats: ";
        for (string c : Data::getKnownFormats())
            msg += c += ", ";
        reportStatus(msg.substr(0, msg.length() - 2));
    }
}

void CmdSet::execute() {
    // determines, which option to set
    std::string line = loadInput("set.history", "What option set?", false, false);
    if (line.find("codec") != std::string::npos)
        DATA->cmds[SET_CODEC]->execute();
    else if (line.find("chunksize") != std::string::npos)
        DATA->cmds[SET_SIZE]->execute();
    else if (line.find("format") != std::string::npos)
        DATA->cmds[SET_FORMAT]->execute();
    else
        reportStatus("Available options: 'codec'', 'chunksize', 'format'");
    state->printVideoState();
}

void CmdLoad::execute(){
    std::string path, out, err, err_msg("Error loading the file: ");
    Document document;
    std::stringstream ssd;
    struct FileInfo finfo;

    // reads the file path
    path = loadInput("paths.history", "Enter a file path:", true, true);
    if (path.empty()) {
        reportError("File path not provided.");
        return;
    }

    // is file ok?
    if (OSHelper::checkFile(path) == -1){
        reportError("Loading the file " + path + " failed");
        return;
    }
    // svaes information in the FileInfo structure
    finfo.fpath = path;
    finfo.extension = "." + OSHelper::getExtension(path);
    finfo.basename = OSHelper::getBasename(path);
    // gain some info about the video
    if (OSHelper::runExternal(out, err, 5,
                               DATA->config.getStringValue("FFPROBE_LOCATION").c_str(), 6,
                               DATA->config.getStringValue("FFPROBE_LOCATION").c_str(),
                               finfo.fpath.c_str(), "-show_streams", "-show_format",
                              "-print_format", "json") < 0) {
        reportError("Error while getting video information.");
        return;
    }
    if (err.find("Invalid data") != std::string::npos) {
        reportError("Invalid video file");
        state->resetFileInfo();
        return;
    }

    // parse the JSON output and save
    if(document.Parse(out.c_str()).HasParseError()) {
        reportError(err_msg + "Parse error");
        return;
    }
    if (!document.HasMember("format")) {
        reportError(err_msg);
        return;
    }
    if(!document["format"].HasMember("bit_rate")) {
        reportError(err_msg);
        return;
    }
    ssd.clear();
    ssd.str(document["format"]["bit_rate"].GetString());
    ssd >> finfo.bitrate;
    finfo.bitrate /= 8;

    if(!document["format"].HasMember("duration")) {
        reportError(err_msg);
        return;
    }
    ssd.clear();
    ssd.str(document["format"]["duration"].GetString());
    ssd >> finfo.duration;

    if(!document["format"].HasMember("size")) {
        reportError(err_msg);
        return;
    }
    ssd.clear();
    ssd.str(document["format"]["size"].GetString());
    ssd >> finfo.fsize;

    if(!document["format"].HasMember("format_name")) {
        reportError(err_msg);
        return;
    }
    finfo.format = document["format"]["format_name"].GetString();

    if(!document.HasMember("streams")) {
        reportError(err_msg);
        return;
    }
    const Value &streams = document["streams"];
    bool found = false;
    if (!streams.IsArray()) {
        reportError("Invalid video file");
        return;
    }
    // codec information
    for(SizeType i = 0; i < streams.Size(); ++i) {
        if (streams[i].HasMember("codec_type") && (streams[i]["codec_type"].GetString() == std::string("video"))) {
            finfo.codec = streams[i]["codec_name"].GetString();
            found = true;
            break;
        }
    }
    if (!found) {
        reportError("Invalid video file");
        return;
    }
    // loads the file to the VideoState
    state->loadFileInfo(finfo);
    reportSuccess(finfo.fpath + " loaded.");
    state->printVideoState();
}

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
    reportSuccess("iiii");
}

void CmdShow::execute() {
    std::string what = loadInput("show.hist", "", false);
    if (what == "jobs") {

    } else if (what.find( "neighbors") != std::string::npos) {
        cursToInfo();
        DATA->neighbors.applyToNeighbors(
                   [&](std::pair<std::string, NeighborInfo *> entry) {
            entry.second->printInfo();
        });
    } else {
        if (state->finfo.fpath.empty()) {
            reportError("Please load the file first.");
            return;
        }
        cursToInfo();
        state->printVideoState();
    }
}

void CmdStart::execute() {
    if (state->finfo.fpath.empty()) {
        reportError("Please load the file first.");
        return;
    }

    if (OSHelper::checkFile(state->finfo.fpath) == -1) {
        reportError("Invalid file.");
        return;
    }

    if (DATA->state.working) {
        reportError("Already in working process.");
        return;
    }
    cursToInfo();
    state->printVideoState();
    refresh();
    if (state->split() == -1) {
        reportError("Error while splitting the video file.");
        OSHelper::rmrDir(state->dir_location.c_str(), false);
        return;
    }
    /*
    if (state->join() == -1) {
        reportError("Error while joining the video file.");
        rmrDir(state->dir_location.c_str(), false);
        return;
    }
    rmrDir(state->dir_location.c_str(), false);
    */
}

void CmdSetCodec::execute() {
    std::string in = loadInput("codecs", "Enter desired output codec:", false);
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
    std::string in = loadInput("", "Enter new chunk size (kB):", false);
    size_t nsize = DATA->config.getIntValue("CHUNK_SIZE");
    std::stringstream ss(in), msg;
    ss >> nsize;
    state->changeChunkSize(nsize * 1024);
    msg << "Chunk size set to: " << nsize;
    reportSuccess(msg.str());
}

void CmdSetFormat::execute() {
    std::string in = loadInput("", "Enter desired output format:", false);
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
    std::string line = loadInput("set.history", "What option set?", false);
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

    path = loadInput("paths.history", "Enter a file path:", true);
    if (path.empty()) {
        reportError("File path not provided.");
        return;
    }
    if (OSHelper::checkFile(path) == -1){
        reportError("Loading the file " + path + " failed");
        return;
    }
    finfo.fpath = path;
    finfo.extension = "." + OSHelper::getExtension(path);
    finfo.basename = OSHelper::getBasename(path);
    if (OSHelper::runExternal(out, err,
                               DATA->config.getStringValue("FFPROBE_LOCATION").c_str(), 6,
                               DATA->config.getStringValue("FFPROBE_LOCATION").c_str(),
                               finfo.fpath.c_str(), "-show_streams", "-show_format", "-print_format", "json") == -1) {
        reportError("Error while getting video information.");
        return;
    }
    if (err.find("Invalid data") != std::string::npos) {
        reportError("Invalid video file");
        state->resetFileInfo();
        return;
    }
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
    state->loadFileInfo(finfo);
    reportSuccess(finfo.fpath + " loaded.");
    state->printVideoState();
}

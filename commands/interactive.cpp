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
    std::string what = loadInput("lists/show_options.list", "", false, false);
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
        state->printTaskState();
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
    state->printTaskState();
    if (state->split() == -1) {
        reportError("Error while splitting the video file.");
        OSHelper::rmrDir(state->dir_location.c_str(), false);
        return;
    }
}

void CmdSetCodec::execute() {
    // reads the input and sets the codec
    std::string in = loadInput("lists/codecs.list", "Enter desired output codec:", false, false);
    if (utilities::isKnown(in, DATA->getKnownCodecs())) {
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
    if (nsize <= 0) {
        reportError("Should be number");
        return;
    }
    // is supposed to be stored in bytes
    state->changeChunkSize(nsize * 1024);
    msg << "Chunk size set to: " << nsize << " kB";
    reportSuccess(msg.str());
}

void CmdSetFormat::execute() {
    // loads the desired container format
    std::string in = loadInput("lists/formats.list", "Enter desired output format:", false, false);
    if (utilities::isKnown(in, DATA->getKnownFormats())) {
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

void CmdSetQuality::execute() {
    // loads the desired container format
    std::string in = loadInput("lists/qualities.list", "Enter desired quality:", false, false);
    if (utilities::isKnown(in, DATA->getKnownQualities())) {
        try {
            DATA->config.strValues.at("QUALITY") = in;
        } catch (std::out_of_range) {
            DATA->config.strValues.emplace("QUALITY", in);
        }

        reportSuccess("Output quality set to: " + in);
    } else {
        reportError(in + ": Not a valid format.");
        std::string msg;
        msg += "Available options: ";
        for (string c : Data::getKnownQualities())
            msg += c += ", ";
        reportStatus(msg.substr(0, msg.length() - 2));
    }
}

void CmdSet::execute() {
    // determines, which option to set
    std::string line = loadInput("lists/set_options.list", "What option set?", false, false);
    if (line.find("codec") != std::string::npos)
        DATA->cmds[CMDS::SET_CODEC]->execute();
    else if (line.find("chunksize") != std::string::npos)
        DATA->cmds[CMDS::SET_SIZE]->execute();
    else if (line.find("format") != std::string::npos)
        DATA->cmds[CMDS::SET_FORMAT]->execute();
    else if (line.find("quality") != std::string::npos)
        DATA->cmds[CMDS::SET_QUALITY]->execute();
    else
        reportStatus("Available options: 'codec'', 'chunksize', 'quality'");
    state->printTaskState();
}

void CmdLoad::execute(){
    // reads the file path
    std::string path(loadInput("paths.history", "Enter a file path:", true, true));
    if (path.empty()) {
        reportError("File path not provided.");
        return;
    }
    state->loadFile(path);
}

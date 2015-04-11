#include "headers/defines.h"
#include "headers/include_list.h"

#include <fstream>
#include <algorithm>


int64_t VideoState::split() {
    // sets busy state
    DATA->state.working = true;
    processed_chunks = 0;
    double sum = 0.0, retval = 0.0, elapsed = 0.0;
    int64_t duration;

    // job with timestamp
    job_id = "job_" + utilities::getTimestamp();
    // absolute path
    std::string path(DATA->config.getStringValue("WD")
                     + std::string("/") + job_id);
    std::string chunk_name;
    char chunk_id[BUF_LENGTH];

    if (OSHelper::prepareDir(path, false) == -1) {
        reportError("Failed to create the job directory.");
        return -1;
    }

    // compute how long one chunk takes, format


    reportStatus("Splitting file: " + finfo.fpath);
    // how many left to receive
    DATA->state.to_recv = chunk_count;
    struct sockaddr_storage my_addr, neighbor_addr, super_addr;

    // gather some neighbors
    if (!DATA->state.enough_neighbors) {
        if (networkHelper::getSuperPeerAddr(super_addr) == -1) {
            reportDebug("Failed to obtain my address.", 2);
        } else {
            if (networkHelper::getMyAddress(super_addr,
                                            my_addr, net_handler) == -1) {
                    reportDebug("Failed to get my adress while contacting peers.", 2);
            } else {
                    if (DATA->neighbors.getRandomNeighbor(neighbor_addr) != 0) {
                            net_handler->gatherNeighbors(
                                            DATA->config.getIntValue("TTL"), my_addr, neighbor_addr);
                    }
            }
        }
    }
    // reports
    utilities::printOverallState(this);

    for (int64_t i = 0; i < chunk_count; ++i) {
        double percent = (double) i / chunk_count;

        // format the command
        printProgress(percent);
        // add the duration of the last chunk
        elapsed += retval;
        snprintf(chunk_id, BUF_LENGTH, "%03lu_splitted", i);
        chunk_name = std::string(chunk_id);
        // possible situation - in the end actually
        if (elapsed > finfo.duration) {
            // how many left to receive actually
            DATA->state.to_recv = i;
            break;
        }

        // creates new TransferInfo structure
        TransferInfo *ti = new TransferInfo(0, job_id, chunk_name,
                                            finfo.extension, o_format,
                                            path + "/" + chunk_name + finfo.extension,
                                            o_codec);

        duration = chunkhelper::createChunk(this,
                    ti, elapsed, &retval);
        //TODO: check
        sum += duration;
        // queue chunk for send
        chunkhelper::pushChunkSend(ti);
        // update state
        utilities::printOverallState(this);
    }
    // 100%
    printProgress(1);
    // report file
    ofs.open(DATA->config.getStringValue("WD") + "/" + job_id + ".out");
    reportTime("Splitting: ", sum / 1000);
    // removes the progress bar
    clearProgress();
    return 0;
}

void VideoState::abort() {
    // TODO:revisit
    reportError("ABORTING process.");
    auto chunks = DATA->chunks_to_send.getValues();
    DATA->chunks_returned.clear();
    DATA->chunks_to_send.clear();
    for (auto ti : chunks) {
        DATA->periodic_listeners.removeIf(
                    [&](Listener *l) {
            return (l->toString().find("TI") !=
                    std::string::npos);
        });
        delete ti;
    }
    clearProgress();
    DATA->state.working = false;
}

int64_t VideoState::join() {
    std::string out, err, list_loc(DATA->config.getStringValue("WD") + "/received/" + job_id + "/join_list.txt"),
            output(DATA->config.getStringValue("WD") + "/" + finfo.basename + "_output" + o_format);
    std::string file, file_item;
    std::ofstream ofs_loc(list_loc);
    char fn[BUF_LENGTH], cmd[BUF_LENGTH];
    long sum_size = 0;
    double duration = 0.0;
    OSHelper::rmFile(output);
    errno = 0;
    reportStatus("Joining the file: " + output);
    snprintf(cmd, BUF_LENGTH, "%s",
             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());

    // create the joining text file
    for (int64_t i = 0; i < chunk_count; ++i) {
        snprintf(fn, BUF_LENGTH, "%03lu_splitted", i);
        file = DATA->config.getStringValue("WD") +
                "/received/" + job_id + "/" + fn + o_format;
        if (OSHelper::getFileSize(file) <= 0) {
            reportError("file: '" + file + "'' is not ok, failed.");
            abort();
            return -1;
        }
        file_item = "file '" + file + "'";
        sum_size += OSHelper::getFileSize(file);
        ofs_loc << file_item << std::endl;
    }
    ofs_loc.flush();
    ofs_loc.close();

    // TODO: check for hang
    std::thread thr(reportFileProgress, output, sum_size);
    // spawn the joining command
    duration = Measured<>::exec_measure(OSHelper::runExternal,
                                        out, err, 1200, cmd, 9, cmd,
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    "-nostdin",
                    output.c_str());
    if ((err.find("failed") != std::string::npos) ||
            (duration < 0)) {
        thr.detach();
        reportError(output + ": Error while joining file.");
        reportError(err);
        clearProgress();
        return -1;
    }
    // wait for end of the process
    thr.join();
    endProcess(duration);
    return 0;
}

void VideoState::reportTime(std::string msg, int64_t time) {
    // outputs to file
    ofs << msg  << time << std::endl;
    reportStatus("The operation took " +
                 utilities::m_itoa(time) + " seconds.");
    ofs.flush();
}

void VideoState::endProcess(int64_t duration) {
    std::ofstream csv_stream(DATA->config.getStringValue("WD") + "/data.csv");
    // 100%
    printProgress(1);
    reportSuccess("Succesfully joined.");
    reportTime("Joining: ", duration / 1000);
    // removes the progress bar
    clearProgress();
    // received chunks no longer needed
    OSHelper::rmrDir(DATA->config.getStringValue("WD") + "/received/", true);
    // is ready
    DATA->state.working = false;
    ofs << "Information about particular chunks:" << std::endl;
    std::vector<TransferInfo *> tis = DATA->chunks_returned.getValues();
    // sorts the chunks
    std::sort(tis.begin(), tis.end(), [&](
              TransferInfo *t1, TransferInfo *t2) {
        return (t1->encoding_time < t2->encoding_time);
    });
    // traverses chunks and reports
    for (auto &ti : tis) {
        ofs << ti->getInfo();
        csv_stream << ti->getCSV();
        delete ti;
    }
    ofs.flush();
    ofs.close();
    csv_stream.flush();
    csv_stream.close();
    DATA->chunks_returned.clear();
}

void VideoState::printVideoState() {
    // prepare the window
    DATA->io_data.info_handler.clear();

    // prints fields...
    if (!finfo.fpath.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("File:",
                                                               finfo.fpath), PLAIN);
    }
    if (!finfo.codec.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Codec:",
                                                               finfo.codec), PLAIN);
    }
    if (finfo.bitrate) {
        DATA->io_data.info_handler.add(utilities::formatString("Bitrate:",
                                                               utilities::m_itoa(finfo.bitrate)), PLAIN);
    }
    if (finfo.duration) {
        DATA->io_data.info_handler.add(utilities::formatString("Duration:",
                                                               utilities::m_itoa(finfo.duration)), PLAIN);
        }
    if (finfo.fsize) {
        DATA->io_data.info_handler.add(utilities::formatString("File size:",
                                                               utilities::m_itoa(finfo.fsize)), PLAIN);
    }
    if (chunk_size) {
        DATA->io_data.info_handler.add(utilities::formatString("Chunk size:",
                                                               utilities::m_itoa(
                                                                   DATA->config.getIntValue("CHUNK_SIZE"))), PLAIN);
    }
    if (!o_codec.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Output codec:",
                                                               o_codec), PLAIN);
    }
    if (!o_format.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Output extension:",
                                                               o_format), PLAIN);
    }
    DATA->io_data.info_handler.print();
}

void VideoState::loadFileInfo(struct FileInfo &finfo) {
    this->finfo = finfo;
    // updates size
    changeChunkSize(DATA->config.getIntValue("CHUNK_SIZE"));
}

void VideoState::resetFileInfo() {
    finfo.fpath = "";
    finfo.codec = "";
    finfo.bitrate = 0;
    finfo.duration = 0;
    finfo.fsize = 0;
    secs_per_chunk = 0;
    chunk_count = 0;
}

void VideoState::changeChunkSize(size_t nsize) {
    // set the size
    chunk_size = nsize;
    // how many seconds *approximately* takes one chunk of this size
    secs_per_chunk = chunk_size  / finfo.bitrate;
    // new chunk count
    chunk_count = (((size_t) finfo.fsize) / chunk_size) + 1;
    DATA->config.intValues.at("CHUNK_SIZE") = nsize;
    utilities::printOverallState(this);
}

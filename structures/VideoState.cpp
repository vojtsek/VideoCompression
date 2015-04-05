#include "headers/defines.h"
#include "headers/include_list.h"

#include <fstream>
#include <algorithm>


int64_t VideoState::split() {
    // sets busy state
    DATA->state.working = true;
    processed_chunks = 0;
    std::string out, err;
    double sum = 0.0;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;

    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH], chunk_id[BUF_LENGTH], current[BUF_LENGTH], msg[BUF_LENGTH], cmd[BUF_LENGTH];
    char dir_name[BUF_LENGTH];

    // job with timestamp
    snprintf(dir_name, BUF_LENGTH, "job_%s",
            utilities::getTimestamp().c_str());
    job_id = std::string(dir_name);
    // absolute path
    std::string path(dir_location + std::string("/") + job_id);
    if (OSHelper::prepareDir(path, false) == -1) {
        reportError("Failed to create the job directory.");
        return -1;
    }

    // compute how long one chunk takes, format
    mins = secs_per_chunk / 60;
    secs = secs_per_chunk % 60;
    snprintf(chunk_duration, BUF_LENGTH, "00:%02lu:%02lu",
             mins, secs);

    reportStatus("Splitting file: " + finfo.fpath);
    // how many left to receive
    DATA->state.to_recv = chunk_count;
    struct sockaddr_storage my_addr, neighbor_addr;

    // gather some neighbors
    if (!DATA->state.enough_neighbors) {
            if (networkHelper::getMyAddress(my_addr, net_handler) == -1) {
                    reportDebug("Failed to get my adress while contacting peers.", 2);
            } else {
                    if (DATA->neighbors.getRandomNeighbor(neighbor_addr) != 0) {
                            net_handler->gatherNeighbors(
                                            DATA->config.getIntValue("TTL"), my_addr, neighbor_addr);
                    }
            }
    }
    // reports
    utilities::printOverallState(this);

    for (int64_t i = 0; i < chunk_count; ++i) {
        double percent = (double) i / chunk_count;

        // format the command
        printProgress(percent);
        elapsed = i * secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        snprintf(current, BUF_LENGTH, "%02lu:%02lu:%02lu", hours, mins, secs);
        snprintf(output, BUF_LENGTH, "%s/%s/%03lu_splitted%s",
                 dir_location.c_str(), job_id.c_str(), i, finfo.extension.c_str());
        snprintf(chunk_id, BUF_LENGTH, "%03lu_splitted", i);
        snprintf(cmd, BUF_LENGTH, "%s",
                 DATA->config.getStringValue("FFMPEG_LOCATION").c_str());
        OSHelper::rmFile(output);
        // spawn the command
        sum += Measured<>::exec_measure(OSHelper::runExternal, out, err, cmd, 12, cmd,
                    "-ss", current,
                    "-i", finfo.fpath.c_str(),
                    "-v", "quiet",
                    "-c", "copy",
                    "-t", chunk_duration,
                    output);
        // failure
        if (err.find("Conversion failed") != std::string::npos) {
            snprintf(msg, BUF_LENGTH, "%s %s %s %s %s %s %s %s\n",
                    DATA->config.getStringValue("FFMPEG_LOCATION").c_str(),
                "-i", finfo.fpath.c_str(),
                "-ss", current,
                "-t", chunk_duration,
                output);
            reportError(msg);
            abort();
            return -1;
        }
        // creates new TransferInfo structure
        TransferInfo *ti = new TransferInfo(OSHelper::getFileSize(output),
                                            job_id, chunk_id, finfo.extension, o_format,
                                            std::string(output), o_codec);
        ti->path = DATA->config.working_dir + "/" + ti->job_id +
                "/" + ti->name + ti->original_extension;
        // queue chunk for send
        chunkhelper::pushChunkSend(ti);
        // update state
        utilities::printOverallState(this);
    }
    // 100%
    printProgress(1);
    // report file
    ofs.open(DATA->config.working_dir + "/" + job_id + ".out");
    reportTime("Splitting: ", sum / 1000);
    // removes the progress bar
    clearProgress();
    return 0;
}

void VideoState::abort() {
    // TODO:revisit
    auto chunks = DATA->chunks_to_send.getValues();
    DATA->chunks_returned.clear();
    DATA->chunks_to_send.clear();
    for (auto ti : chunks) {
        delete ti;
    }
    clearProgress();
    DATA->state.working = false;
}

int64_t VideoState::join() {
    std::string out, err, list_loc(DATA->config.working_dir + "/received/" + job_id + "/join_list.txt"),
            output(DATA->config.working_dir + "/" + finfo.basename + "_output" + o_format);
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
        file = DATA->config.working_dir +
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
    duration = Measured<>::exec_measure(OSHelper::runExternal, out, err, cmd, 8, cmd,
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    output.c_str());
    if (err.find("failed") != std::string::npos) {
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
    std::ofstream csv_stream(DATA->config.working_dir + "/data.csv");
    // 100%
    printProgress(1);
    reportSuccess("Succesfully joined.");
    reportTime("Joining: ", duration / 1000);
    // removes the progress bar
    clearProgress();
    // received chunks no longer needed
    OSHelper::rmrDir(DATA->config.working_dir + "/received/", true);
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
    // how many seconds *aproximately* takes one chunk of this size
    secs_per_chunk = chunk_size  / finfo.bitrate;
    // new chunk count
    chunk_count = (((size_t) finfo.fsize) / chunk_size) + 1;
}

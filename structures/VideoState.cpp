#include "headers/defines.h"
#include "headers/include_list.h"

#include <fstream>
#include <algorithm>


int32_t VideoState::split() {
    DATA->state.working = true;
    processed_chunks = false;
    std::string out, err;
    double sum = 0.0;
    size_t elapsed = 0, mins = 0, secs = 0, hours = 0;
    char chunk_duration[BUF_LENGTH], output[BUF_LENGTH], chunk_id[BUF_LENGTH], current[BUF_LENGTH], msg[BUF_LENGTH], cmd[BUF_LENGTH];
    char dir_name[BUF_LENGTH];
    sprintf(dir_name, "job_%s", utilities::getTimestamp().c_str());
    job_id = std::string(dir_name);
    std::string path(dir_location + std::string("/") + job_id);
    if (OSHelper::prepareDir(path, false) == -1) {
        reportError("Failed to create the job directory.");
        return (-1);
    }

    mins = secs_per_chunk / 60;
    secs = secs_per_chunk % 60;
    snprintf(chunk_duration, BUF_LENGTH, "00:%02d:%02d", mins, secs);
    reportStatus("Splitting file: " + finfo.fpath);
    DATA->state.to_recv = c_chunks;
    struct sockaddr_storage my_addr, neighbor_addr;
    if (networkHelper::getMyAddress(my_addr, net_handler) == -1) {
        reportDebug("Failed to get my adress while contacting peers.", 2);
    } else {
        if (DATA->neighbors.getRandomNeighbor(neighbor_addr) != -1) {
            net_handler->gatherNeighbors(
                    DATA->config.getIntValue("TTL"), my_addr, neighbor_addr);
        }
    }
    utilities::printOverallState(this);
    for (uint32_t i = 0; i < c_chunks; ++i) {
        double percent = (double) i / c_chunks;

        printProgress(percent);
        elapsed = i * secs_per_chunk;
        hours = elapsed / 3600;
        elapsed = elapsed % 3600;
        mins = elapsed / 60;
        secs = elapsed % 60;
        snprintf(current, BUF_LENGTH, "%02d:%02d:%02d", hours, mins, secs);
        snprintf(output, BUF_LENGTH, "%s/%s/%03d_splitted%s",
                 dir_location.c_str(), job_id.c_str(), i, finfo.extension.c_str());
        snprintf(chunk_id, BUF_LENGTH, "%03d_splitted", i);
        snprintf(cmd, BUF_LENGTH,
                 DATA->config.getStringValue("FFMPEG_LOCATION").c_str());
       // cursToStatus();
        OSHelper::rmFile(output);
        sum += Measured<>::exec_measure(OSHelper::runExternal, out, err, cmd, 12, cmd,
                    "-ss", current,
                    "-i", finfo.fpath.c_str(),
                    "-v", "quiet",
                    "-c", "copy",
                    "-t", chunk_duration,
                    output);
        if (err.find("Conversion failed") != std::string::npos) {
            sprintf(msg, "%s %s %s %s %s %s %s %s\n",
                    DATA->config.getStringValue("FFMPEG_LOCATION").c_str(),
                "-i", finfo.fpath.c_str(),
                "-ss", current,
                "-t", chunk_duration,
                output);
            reportError(msg);
            abort();
            return -1;
        }
        TransferInfo *ti = new TransferInfo(OSHelper::getFileSize(output),
                                            job_id, chunk_id, finfo.extension, o_format,
                                            std::string(output), o_codec);
        ti->path = DATA->config.working_dir + "/" + ti->job_id +
                "/" + ti->name + ti->original_extension;
        chunkhelper::pushChunkSend(ti);
        utilities::printOverallState(this);
    }
    printProgress(1);
    ofs.open(DATA->config.working_dir + "/" + job_id + ".out");
    reportTime("Splitting: ", sum / 1000);
    clearProgress();
    return 0;
}

void VideoState::abort() {
    //TODO: delete structures
    clearProgress();
    DATA->state.working = false;
}

int32_t VideoState::join() {
    std::string out, err, list_loc(DATA->config.working_dir + "/received/" + job_id + "/join_list.txt"),
            output(DATA->config.working_dir + "/" + finfo.basename + "_output" + o_format);
    std::string file, file_item;
    std::ofstream ofs_loc(list_loc);
    char fn[BUF_LENGTH], cmd[BUF_LENGTH];
    long sum_size = 0;
    double duration = 0;
    unlink(output.c_str());
    errno = 0;
    reportStatus("Joining the file: " + output);
    snprintf(cmd, BUF_LENGTH,
             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());

    // create the joining text file
    for (uint32_t i = 0; i < c_chunks; ++i) {
        snprintf(fn, BUF_LENGTH, "%03d_splitted", i);
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
    thr.join();
    endProcess(duration);
    return 0;
}

void VideoState::reportTime(std::string msg, int32_t time) {
    ofs << msg  << time << std::endl;
    reportStatus("The operation took " +
                 utilities::m_itoa(time) + " seconds.");
}

void VideoState::endProcess(int32_t duration) {
    std::ofstream csv_stream(DATA->config.working_dir + "/data.csv");
    printProgress(1);
    reportSuccess("Succesfully joined.");
    reportTime("Joining: ", duration / 1000);
    clearProgress();
    OSHelper::rmrDir(DATA->config.working_dir + "/received/", true);
    DATA->state.working = false;
    ofs << "Information about particular chunks:" << std::endl;
    std::vector<TransferInfo *> tis = DATA->chunks_returned.getValues();
    std::sort(tis.begin(), tis.end(), [&](
              TransferInfo *t1, TransferInfo *t2)
    {return (t1->encoding_time < t2->encoding_time);});
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
    DATA->chunks_to_send.clear();
}

void VideoState::printVideoState() {
    DATA->io_data.info_handler.clear();
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
    changeChunkSize(DATA->config.getIntValue("CHUNK_SIZE"));
}

void VideoState::resetFileInfo() {
    finfo.fpath = "";
    finfo.codec = "";
    finfo.bitrate = 0.0;
    finfo.duration = 0.0;
    finfo.fsize = 0.0;
    secs_per_chunk = 0;
    c_chunks = 0;
}

void VideoState::changeChunkSize(size_t nsize) {
    chunk_size = nsize;
    secs_per_chunk = chunk_size  / finfo.bitrate;
    c_chunks = (((size_t) finfo.fsize) / chunk_size) + 1;
}

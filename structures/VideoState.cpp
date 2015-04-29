#include "headers/defines.h"
#include "headers/include_list.h"

#include <fstream>
#include <algorithm>
#include <dirent.h>


int64_t VideoState::split() {
    // sets busy state
    DATA->state.working = true;
    processed_chunks = 0;
    int64_t duration;

    // job with timestamp
    job_id = "job_" + utilities::getTimestamp();
        ofs.open(LOG_PATH + PATH_SEPARATOR + job_id + ".out");
    if (DATA->config.encode_first) {
            std::string out,err;
            char cmd[BUF_LENGTH];
            snprintf(cmd, BUF_LENGTH, "%s",
                                             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());
            duration = Measured<>::exec_measure(
                                    OSHelper::runExternal, out, err,
                                    finfo.duration * 2, cmd, 13	, cmd,
                                             "-i", finfo.fpath.c_str(),
                                             "-c:v", "libx264",
                                             "-preset",DATA->config.getStringValue("QUALITY").c_str(),
                                             "-c:a", "copy",
                                             "-nostdin",
                                             "-qp", "0",
                                             "encodingTest.mkv");
            OSHelper::rmFile("encodingTest.mkv");

            ofs << "As whole: " << duration << std::endl;
    }
    // absolute path
    dir_location = SPLITTED_PATH;
    std::string chunk_name;
    char chunk_id[BUF_LENGTH];

    start_time = std::chrono::system_clock::now();
    if (OSHelper::prepareDir(dir_location, false) == -1) {
        reportError("Failed to create the job directory.");
        return -1;
    }

    // compute how long one chunk takes, format


    reportStatus("Splitting file: " + finfo.fpath);
    // how many left to receive
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

    std::string out,err;
    char cmd[BUF_LENGTH], chunk_duration[BUF_LENGTH];
    snprintf(cmd, BUF_LENGTH, "%s",
             DATA->config.getStringValue("FFMPEG_LOCATION").c_str());
    snprintf(chunk_duration, BUF_LENGTH, "%lu", secs_per_chunk);
    std::string output = SPLITTED_PATH + PATH_SEPARATOR +
                job_id + "_%d" + finfo.extension;
    // spawn the splitting command
    duration = Measured<>::exec_measure(OSHelper::runExternal,
                                        out, err, 60, cmd, 15, cmd,
                    "-i", finfo.fpath.c_str(),
                    "-f", "segment",
                    "-segment_time", chunk_duration,
                    "-reset_timestamps", "1",
                    "-c", "copy",
                    "-nostdin", "-map", "0",
                    output.c_str());
    if ((err.find("failed") != std::string::npos) ||
            (duration < 0)) {
        reportError(output + ": Error while splitting file.");
        clearProgress();
        return -1;
    }

    DIR * dirp;
    struct dirent * entry;
    chunk_count = 0;

    dirp = opendir(std::string(SPLITTED_PATH).c_str());
    if (dirp == NULL) {
        reportError("Failed to read splitted data");
        abort();
    }
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG) {
            std::string name(entry->d_name);
            if (name.find(job_id) != std::string::npos) {
             chunk_count++;
            }
        }
    }
    closedir(dirp);

    for (int64_t i = 0; i < chunk_count; ++i) {
        // the process was aborted by the user
        if (aborted) {
            aborted = false;

            break;
        }
        snprintf(chunk_id, BUF_LENGTH, "%s_%lu", job_id.c_str(), i);
        chunk_name = std::string(chunk_id);
        // creates new TransferInfo structure
        TransferInfo *ti = new TransferInfo(0, job_id, chunk_name,
                                            finfo.extension, o_format,
                                            dir_location + PATH_SEPARATOR + chunk_name + finfo.extension,
                                            o_codec);

        ti->quality = DATA->config.getStringValue("QUALITY");
        ti->chunk_size = OSHelper::getFileSize(ti->path);
        ti->time_left = ti->chunk_size / (1024 * 1000) * DATA->getQualityCoeff(ti->quality);
        // queue chunk for send
        chunkhelper::pushChunkSend(ti);
        // update state
        utilities::printOverallState(this);
    }
    // report file
       reportTime("Splitting: ", duration / 1000);
    return 0;
}

void VideoState::abort() {
    reportError("ABORTING process.");
    aborted = true;
    auto chunks = DATA->chunks_to_send.getValues();
    DATA->chunks_returned.clear();
    for (auto ti : chunks) {
        // remove waiting chunks
        DATA->periodic_listeners.removeIf(
                    [&](Listener *l) {
            return (l->toString().find("TI") !=
                    std::string::npos);
        });
        chunkhelper::trashChunk(ti, true);
    }
    clearProgress();
    // adjusts the state variable
    DATA->state.working = false;
    // removes useless files
    OSHelper::rmrDir(dir_location, true);
    utilities::printOverallState(this);
    reset();
}

int64_t VideoState::join() {
    std::string out, err,
            list_loc(RECEIVED_PATH + PATH_SEPARATOR + "join_list.txt"),
            output(DATA->config.getStringValue("WD") + PATH_SEPARATOR + finfo.basename + "_output" + o_format);
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
        snprintf(fn, BUF_LENGTH, "%s_%lu", job_id.c_str(), i);
        file = RECEIVED_PATH + PATH_SEPARATOR + fn + o_format;
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

    std::thread thr(reportFileProgress, output, sum_size);
    // spawn the joining command
    duration = Measured<>::exec_measure(OSHelper::runExternal,
                                        out, err, 500, cmd, 9, cmd,
                    "-f", "concat",
                    "-i", list_loc.c_str(),
                    "-c", "copy",
                    "-nostdin",
                    output.c_str());
    // (err.find("failed") != std::string::npos) <- does not have to be always true
    if (duration < 0) {
        thr.detach();
        reportError(output + ": Error while joining file.");
        reportError(err);
        clearProgress();
        abort();
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
    std::ofstream csv_stream(LOG_PATH + PATH_SEPARATOR +
                             job_id + "_data.csv");
    // 100%
    if (DATA->state.interact) {
            printProgress(1);
            clearProgress();
    }
    reportSuccess("Succesfully joined.");
    reportTime("Joining: ", duration / 1000);
    // removes the progress bar
    // received chunks no longer needed
    OSHelper::rmrDir(RECEIVED_PATH, true);
    // is ready
    DATA->state.working = false;
    ofs << "Information about particular chunks:" << std::endl;
    std::vector<TransferInfo *> tis = DATA->chunks_returned.getValues();
    // sorts the chunks
    std::sort(tis.begin(), tis.end(), [&](
              TransferInfo *t1, TransferInfo *t2) {
        return (t1->time_per_kb < t2->time_per_kb);
    });

    double sending_sum(0), receiving_sum(0), encoding_sum(0),
            sent_times(0);
    // traverses chunks and reports
    for (auto &ti : tis) {
        sending_sum += ti->sending_time;
        receiving_sum += ti->receiving_time;
        encoding_sum += ti->encoding_time;
        sent_times += ti->sent_times;
        ofs << ti->getInfo();
        csv_stream << ti->getCSV();
        chunkhelper::trashChunk(ti, true);
    }
    ofs << "Time: " << (std::chrono::duration_cast<std::chrono::milliseconds>
                                        (std::chrono::system_clock::now() - start_time)).count() << std::endl;
    sending_sum /= 1000 * chunk_count;
    receiving_sum /= 1000 * chunk_count;
    encoding_sum /= 1000 * chunk_count;
    sent_times /= chunk_count;
    ofs << sending_sum << "," << receiving_sum << ","
        << encoding_sum << "," << sent_times << ", "
        << DATA->neighbors.getBiggestDifference() << ", " << chunk_count;
    ofs.flush();
    ofs.close();
    csv_stream.flush();
    csv_stream.close();
    DATA->chunks_returned.clear();
    reset();
}

void VideoState::reset() {
    resetFileInfo();
    aborted = false;
    DATA->state.working = false;
    chunk_count = 0;
    processed_chunks = 0;
    dir_location = "";
    changeChunkSize(DATA->config.getIntValue("CHUNK_SIZE") * 1024);
    std::unique_lock<std::mutex> lck(DATA->m_data.interact_mtx, std::defer_lock);
    // notify in non-interactive mode
    lck.lock();
    lck.unlock();
    DATA->m_data.interact_cond.notify_all();
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
        DATA->io_data.info_handler.add(utilities::formatString("Quality:",
                                                               DATA->config.getStringValue("QUALITY")), PLAIN);
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
        DATA->io_data.info_handler.add(utilities::formatString("Chunk count:",
                                                               utilities::m_itoa(chunk_count)), PLAIN);
    }
    if (!o_codec.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Output codec:",
                                                               o_codec), PLAIN);
    }
    if (!o_format.empty()) {
        DATA->io_data.info_handler.add(utilities::formatString("Output format:",
                                                               o_format), PLAIN);
    }
    DATA->io_data.info_handler.print();
}

void VideoState::loadFileInfo(struct FileInfo &finfo) {
    this->finfo = finfo;
    // updates size
    changeChunkSize(DATA->config.getIntValue("CHUNK_SIZE") * 1024);
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
    // set the size [bytes]
    chunk_size = nsize;
    // how many seconds *approximately* takes one chunk of this size
    if (finfo.bitrate) {
            secs_per_chunk = chunk_size  / finfo.bitrate;
    }
    // new chunk count
    chunk_count = (((size_t) finfo.fsize) / chunk_size) + 1;
    DATA->config.intValues.at("CHUNK_SIZE") = nsize / 1024;
    utilities::printOverallState(this);
}

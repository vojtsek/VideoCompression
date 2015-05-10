#ifndef TRANSFERINFO_H
#define TRANSFERINFO_H

#include "structures/structures.h"
#include "headers/utilities.h"

/*!
 * \brief The TransferInfo struct
 * Holds information about particular chunks.
 * Able to be sent over the network and
 * listen to periodic events.
 */
struct TransferInfo : public Listener, Sendable {
    //! whether the chunk has source address assigned
    bool addressed;
    //! indicates, if currently assigned
    bool assigned;
    //! file size in bytes
    int64_t chunk_size;
    //! how many ticks before the checking interval ends
    int64_t time_left;
    //! decrease after timeout is up
    int64_t tries_left;
    //! how many seconds took sending for encoding
    int64_t sending_time;
    //! how many seconds took receiving the chunk back
    int64_t receiving_time;
    //! how many times has been the chunk sent
    int64_t sent_times;
    //! how long took take encoding process
    int64_t encoding_time;
    //! duration of the video in seconds
    int64_t duration;
    //! how many times failed to send, for local purposes only(not transfered)
    int64_t tries_sent;
    //! represents time unit per kilobyte
    double time_per_kb;
    //! start offset
    double start;
    //! address assigned when distributing
    struct sockaddr_storage address;
    //! address of the source node, should be returned to
    struct sockaddr_storage src_address;
    //! string timestamp of beginning the job
    std::string job_id;
    //! file basename without extension
    std::string name;
    //! original file extension (includes dot)
    std::string original_extension;
    //! determines desired container format
    std::string desired_extension;
    //! determines quality of encoding
    std::string quality;
    /*!
     * \brief path to the associated file
     * valid when created, pushed to process and when returned
     * (in the time when the file is processed on the computation node,
     * it still points to the old one)
     */
    std::string path;
    //! codec to be used when encoding
    std::string output_codec;
    //! string timestamp to measure time
    std::string timestamp;

    /*!
     * \brief getInfo gets the file information
     * \return string info about the chunk
     */
    std::string getInfo();

    /*!
     * \brief getCSV get information as comma separated values
     * \return string with csv info
     * Format:
     *
     */
    std::string getCSV();

    std::vector<std::string> run_args;
    /*!
     * \brief invoke metod called periodically, checks the processng time
     * \param handler reference to the NetworkHandler
     * Each time decreases counter, when time is up,
     * tries given of times to contact neihbor, if it is
     * not respoding or too many times, resends the chunk
     */
    virtual void invoke(NetworkHandler &handler);

    /*!
     * \brief toString gets a string id of the chunk
     * \return string id
     */
    virtual std::string toString();

    /*!
     * \brief send handles sending over the network field by field
     * \param fd file descriptor assciated with the connection
     * \return zero on succes
     */
    virtual int64_t send(int64_t fd);

    /*!
     * \brief send handles sending over the network field by field
     * \param fd file descriptor assciated with the connection
     * \return zero on succes
     */
    virtual int64_t receive(int64_t fd);

    /*!
     * \brief print prints information about the chunk using getInfo()
     */
    void print();

    /*!
     * \brief init set default values to fields
     */
    void init(int64_t size,
              std::string ji, std::string n, std::string oe, std::string de,
              std::string p, std::string oc);

    TransferInfo(): addressed(false) {
        init(0, "", "", "", "", "", "");
    }

    TransferInfo(struct sockaddr_storage addr, int64_t size,
                 std::string ji, std::string n, std::string oe, std::string de,
                 std::string p, std::string oc): addressed(true) {
        init(size, ji, n, oe, de, p, oc);
        src_address = addr;
    }

    TransferInfo(int64_t size,
                 std::string ji, std::string n, std::string oe, std::string de,
                 std::string p, std::string oc): addressed(false) {
        init(size, ji, n, oe, de, p, oc);
    }

    virtual ~TransferInfo() {}
};


#endif // TRANSFERINFO_H

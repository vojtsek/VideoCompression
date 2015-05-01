#ifndef SINGLETONS_H
#define SINGLETONS_H

#include <atomic>
#include <algorithm>
#include <unordered_map>
#include "structures/WindowPrinter.h"
#include "headers/enums_types.h"
#include "structures/SynchronizedQueue.h"
#include "structures/NeighborStorage.h"
#include "structures/structures.h"

struct TransferInfo;
struct Listener;

/*!
 * \brief The State struct
 * holds state of the programme
 */
struct State {
    //! whether has enough neighbors
    bool enough_neighbors = false;
    //! if is processing file currently
    bool working = false;
    //! indicates interactivity
    bool interact = true;
    //! whether provided home directory explicitly
    bool wd_provided = false;
    //! indicates end of execution, so prevents further actions i.e. making connection, accept commands
    bool quitting = false;
    //! file to load when don't run interactively
    std::string file_path = "";
};

/*!
 * \brief The IO_Data struct
 * holds information regarding IO stuff
 */
struct IO_Data {
    IO_Data(std::string log_location): info_handler(STATIC, false, TOP, log_location),
        status_handler(UP, true, BOTTOM, log_location) {}
    //! handlers corresponding to windows
    WindowPrinter info_handler;
    //! handlers corresponding to windows
    WindowPrinter status_handler;
    //! y coordinate of the status row
    int64_t status_y = 0;
    //! y coordinate of the percent row
    int64_t perc_y = 0;
    //! y coordinate of the question row
    int64_t question_y = 0;
    //! pointer to status window
    WINDOW *status_win;
    //! pointer to info window
    WINDOW *info_win;

    /*!
     * \brief changeLogLocation updates location of the status log file
     * \param log_location new locaion to be set
     */
    void changeLogLocation(std::string log_location) {
        status_handler.changeLogLocation(log_location);
    }
};

/*!
 * \brief The Mutexes_Data struct holds some mutexes and variables,
 * to synchronize IO
 */
struct Mutexes_Data {
    //! mutex to synchronize input
    std::mutex I_mtx;
    //! mutex to synchronize output
    std::mutex O_mtx;
    //! mutex to synchronize reporting
    std::mutex report_mtx;
    //! mutex to synchronize non interactivity
    std::mutex interact_mtx;
    //! condition variable to handle IO
    std::condition_variable IO_cond;
    //! condition_variable to maintain non-interactive splitting
    std::condition_variable interact_cond;
    //! boolean to help control using input
    bool using_I = false;
    //! boolean to help control using output
    bool using_O = false;
};

/*!
 * \brief The Configuration struct holds the configuration
 * provides methods to access it
 * filled in from configuration file
 */
struct Configuration {
    //! whether the node should act like superpeer
    bool is_superpeer = false;
    //! whether using only IPv4 addresses
    bool IPv4_ONLY = false;
    //! determines if accept chunks to encode while working
    bool serve_while_working = false;
    //! if encode video as a whole
    bool encode_first = false;
    //! level of debug messages to show
    int64_t debug_level = 0;
    //! map of integer values
    std::map<std::string, int64_t> intValues;
    //! map of string values
    std::map<std::string, std::string> strValues;
    //! IP address of the interface host is communicating on
    MyAddr my_IP;
    //! path to the config file
    std::string location;

    /*!
     * \brief getIntValue returns the integer value
     * \param key name of the field
     * \return value of the field
     * If nonexisting field, returns zero.
     */
    int64_t getIntValue(std::string key);

    /*!
     * \brief getStringValue returns the string value
     * \param key name of the field
     * \return value of the field
     * If nonexisting field, returns empty string.
     */
    std::string getStringValue(std::string key);

    /*!
     * \brief setValue sets the configuration value
     * \param key nameof the value
     * \param val new value
     * \param force whether remove old value, if any
     * \return true if successfully inserted
     */
    bool setValue(std::string key, int64_t val, bool force);

    /*!
     * \brief setValue sets the configuration value
     * \param key nameof the value
     * \param val new value
     * \param force whether remove old value, if any
     * \return true if successfully inserted
     */
    bool setValue(std::string key, std::string val, bool force);
};

/*!
 * \brief The Data struct
 * Globally accessible singleton structure,
 * provides access to globally used fields.
 */
struct Data {
    /*!
     * \brief getInstance gets the instance
     * \param log_location location of file to store status log
     * \return pointer to Data instance,
     * Makes sure only one instance exist.
     */
    static std::shared_ptr<Data> getInstance(std::string log_location) {
        if(!inst) {
            inst = std::shared_ptr<Data>(new Data(log_location));
        }
        return inst;
    }

    //! storage containing interactive commands mapped to enum values
    cmd_storage_t cmds;
    //! storage containing network commands mapped to enum values
    net_cmd_storage_t net_cmds;
    //! IO_Data instance, handles IO
    IO_Data io_data;
    //! Mutexes_Data instance, synchronizes IO
    Mutexes_Data m_data;
    //! holds the configuration
    Configuration config;
    //! holds the state
    State state;
    //! maintains the neighbor list
    NeighborStorage neighbors;
    //! queue with synchronized access, holds chunks to be sent
    SynchronizedQueue<TransferInfo> chunks_to_send;
    //! queue with synchronized access, holds chunks to be encoded
    SynchronizedQueue<TransferInfo> chunks_to_encode;
    //! storage which holds the chunks received to encode, mapping them to strings
    SynchronizedMap<TransferInfo> chunks_received;
    //! storage which holds the chunks which returned encoded, mapping them to strings
    SynchronizedMap<TransferInfo> chunks_returned;
    //! holds structures inheriting Listener class, invokes them periodically
    SynchronizedMap<Listener> periodic_listeners;
    //! storage for chunks that were created during the split

    //! known codecs to encode into
    static std::vector<std::string> getKnownCodecs() {
        return {"libx264", "msmpeg"};
    }

    //! containers which could wrap the resulting file
    static std::vector<std::string> getKnownFormats() {
        return {"avi", "mkv", "ogg"};
    }

    //! determines encoding quality
    static std::vector<std::string> getKnownQualities() {
        return {"ultrafast", "superfast", "veryfast", "faster", "fast",
                    "medium", "slow", "slower", "veryslow"};
    }

    /*!
     * \brief getCmdMapping gets mapping of keys to commands.
     * \return map providing desired mapping
     */
    static std::map<wchar_t, CMDS> getCmdMapping() {
        return {
            { KEY_F(6), CMDS::SHOW },
            { KEY_F(7), CMDS::START },
            { KEY_F(8), CMDS::LOAD },
            { KEY_F(9), CMDS::SET },
            { KEY_F(10), CMDS::ABORT_C },
            { KEY_UP, CMDS::SCROLL_UP },
            { KEY_DOWN, CMDS::SCROLL_DOWN }
        };
    }

    double getQualityCoeff(std::string q) {
        if (std::find(getKnownQualities().begin(), getKnownQualities().end(), q)
                == getKnownQualities().end()) {
            return 10.0;
        }
        static std::map<std::string, double> coeffs = {
            {"veryslow", 10.0},
            {"slower", 9.5},
            {"slow", 8.0},
            {"medium", 7.5},
            {"fast", 7.0},
            {"faster", 6.5},
            {"veryfast", 6.0},
            {"superfast", 5.5},
            {"ultrafast", 5.0}
        };

        return coeffs[q];
    }

    Data(std::string log_location) : io_data(log_location) {}

private:
    static std::shared_ptr<Data> inst;
};


#endif // SINGLETONS_H

#ifndef ENUMS_TYPES_H
#define ENUMS_TYPES_H
#include <map>

class Command;
class NetworkCommand;
class NeighborInfo;

//! type of the message to be shown by WindowPrinter, distinguishes colour
enum MSG_T { ERROR, PLAIN, SUCCESS, DEBUG };

//! describes state of the node when communicating
enum class RESPONSE_T : int64_t { ACK_FREE, ACK_BUSY, AWAITING, ABORT };

//! used to send commands over the network, saved in the map
enum class CMDS : char { TERM, DEFCMD, SHOW, START, LOAD,
            SET, SET_CODEC, SET_SIZE, SET_FORMAT, SET_QUALITY,
            SCROLL_UP, SCROLL_DOWN, ABORT_C,
          ASK_PEER, ASK_HOST,
          CONFIRM_PEER, CONFIRM_HOST,
          PING_PEER, PING_HOST,
            GOODBYE_PEER, GOODBYE_HOST, SAY_GOODBYE,
          DISTRIBUTE_PEER, DISTRIBUTE_HOST,
            GATHER_PEER, GATHER_HOST,
          RETURN_PEER, RETURN_HOST };

//! determines the WindowPrinter type
enum PRINTER_DIRECTION {UP, DOWN, STATIC};

//! determines the WindowPrinter type
enum PRINTER_START {TOP, BOTTOM};

//! type to store configuration
typedef std::map<std::string, std::string> configuration_t;

//! type to store local commands
typedef std::map<CMDS, Command *> cmd_storage_t;

//! type to store network commands
typedef std::map<CMDS, NetworkCommand *> net_cmd_storage_t;

//! type that is stored in the WindowPrinter queue
typedef std::pair<std::string, MSG_T> printable_pair_T;

//! type to store neighbors
typedef std::map<std::string, NeighborInfo *> neighbor_storageT;
#endif // ENUMS_TYPES_H

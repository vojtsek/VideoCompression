#ifndef ENUMS_TYPES_H
#define ENUMS_TYPES_H

class Command;
class NetworkCommand;
class NeighborInfo;

enum MSG_T { ERROR, PLAIN, SUCCESS, DEBUG };
enum CONN_T { OUTGOING, INCOMING };
enum RESPONSE_T { ACK_FREE, ACK_BUSY, AWAITING, ABORT };
enum CMDS { TERM, DEFCMD, SHOW, START, LOAD,
            SET, SET_CODEC, SET_SIZE, SET_FORMAT,
          ASK_PEER, ASK_HOST,
          CONFIRM_PEER, CONFIRM_HOST,
          PING_PEER, PING_HOST,
          DISTRIBUTE_PEER, DISTRIBUTE_HOST,
          RETURN_PEER, RETURN_HOST };
enum PRINTER_DIRECTION {UP, DOWN, STATIC};
enum PRINTER_START {TOP, BOTTOM};

typedef std::map<std::string, std::string> configuration_t;
typedef std::map<CMDS, Command *> cmd_storage_t;
typedef std::map<CMDS, NetworkCommand *> net_cmd_storage_t;
typedef std::pair<std::string, MSG_T> printable_pair_T;
typedef std::map<std::string, NeighborInfo *> neighbor_storageT;

#endif // ENUMS_TYPES_H

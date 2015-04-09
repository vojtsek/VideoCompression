#include "headers/commands.h"
#ifndef INTER_CMD_H
#define INTER_CMD_H

/*!
 * \brief The CmdDef class
 * Default command, catches unknown shortcuts
 */
class CmdDef: public Command {
public:
    CmdDef(): Command(nullptr) {}
    virtual void execute();
};

/*!
 * \brief The CmdHelp class
 * shows help
 */
class CmdHelp: public Command {
public:
    CmdHelp(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdScrollUp class
 * scroll the information panel one step up
 */
class CmdScrollUp: public Command {
public:
    CmdScrollUp(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdScrollDown class
 * scroll the information panel one step down
 */
class CmdScrollDown: public Command {
public:
    CmdScrollDown(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdShow class
 * shows desired information, loads the input first
 */
class CmdShow: public NetworkCommand {
public:
    CmdShow(VideoState *st, NetworkHandler *hndl): NetworkCommand(st, hndl) {}
    virtual bool execute(int64_t, struct sockaddr_storage &, void *);
    virtual void execute();
    virtual std::string getName() {
        return("Show");
    }
};

/*!
 * \brief The CmdAbort class aborts current execution
 * of video encoding process
 */
class CmdAbort: public Command {
public:
    CmdAbort(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdStart class
 * starts the computing process,
 * does some checks
 */
class CmdStart: public Command {
public:
    CmdStart(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdSet class
 * loads input and sets the desired parameter to the inputted value
 * uses helper commands
 */
class CmdSet: public Command {
public:
    CmdSet(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdLoad class
 * loads the video file
 */
class CmdLoad: public Command {
public:
    CmdLoad(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdSetCodec class
 * sets the output codes, called by CmdSet
 */
class CmdSetCodec: public Command {
public:
    CmdSetCodec(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdSetFormat class
 * sets the output container, called by CmdSet
 */
class CmdSetFormat: public Command {
public:
    CmdSetFormat(VideoState *st): Command(st) {}
    virtual void execute();
};

/*!
 * \brief The CmdSetChSize class
 * sets the chunk size, called by CmdSet
 */
class CmdSetChSize: public Command {
public:
    CmdSetChSize(VideoState *st): Command(st) {}
    virtual void execute();
};


#endif

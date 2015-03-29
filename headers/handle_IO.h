#include <string>
#ifndef HADLEIO_H
#define HANDLEIO_H

int32_t getLine(char *line, int32_t len,
                const std::string &histf, bool save, bool changeable);
void initCurses();
void cursToInfo();
void cursToPerc();
void cursToCmd();
void cursToStatus();
void cursToQuestion();
void clearNlines(int32_t n);
void clearProgress();
void reportFileProgress(const std::string &file, long desired);
void cursorToX(int32_t nx);
void reportError(const std::string &err);
void reportSuccess(const std::string &msg);
void reportStatus(const std::string &msg);
void reportDebug(const std::string &msg, int32_t lvl);
void printProgress(double percent);
std::string loadInput(const std::string &histf,
                      const std::string &msg, bool save, bool changeable);

#endif

#ifndef HADLEIO_H
#define HANDLEIO_H

int getLine(char *line, int len, const std::string &histf, bool save);    void initCurses();
void cursToInfo();
void cursToPerc();
void cursToCmd();
void cursToStatus();
void cursToQuestion();
void clearNlines(int n);
void clearProgress();
void reportFileProgress(const std::string &file, long desired);
void cursorToX(int nx);
void reportError(const std::string &err);
void reportSuccess(const std::string &msg);
void reportStatus(const std::string &msg);
void reportDebug(const std::string &msg, int lvl);
void printProgress(double percent);
void reportTime(const std::string &file, double time);
std::string loadInput(const std::string &histf, const std::string &msg, bool save);

#endif

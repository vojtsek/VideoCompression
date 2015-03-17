#include <string>
#include <iostream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <vector>
#include <deque>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <curses.h>

#include "headers/enums_types.h"
#include "structures/WindowPrinter.h"
#include "structures/singletons.h"

#ifndef DEFINES_H
#define DEFINES_H
#define DEBUG_LEVEL 2
#define STATUS_LENGTH 10
#define WD "/home/vojcek/WD"
#define INFO_LINES 15
#define SUPERPEER_ADDR "::ffff:127.0.0.1"
#define LINE_LENGTH 80
#define BUF_LENGTH 512
#define DEFAULT 1
#define RED 2
#define GREEN 3
#define GREY 4
#define BLUE 5
#define YELLOWALL 6
#define GREYALL 7
#define CYANALL 8
#define INVERTED 9
#define COLOR_GREY 100
#define BG 20
#define BG_COL COLOR_BLACK
#define ST_Y 10
#define DATA Data::getInstance("log")
#define show(x, y) wprintw(DATA->io_data.info_win, "%15s%35s\n", x, y);

#endif

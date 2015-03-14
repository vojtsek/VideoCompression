﻿#include "commands.h"
#include "include_list.h"
#include "handle_IO.h"
#include "defines.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>
#include <map>
#include <utility>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

using namespace std;
using namespace utilities;

void Command::execute() {
    cursToInfo();
}


int NetworkCommand::connectPeer(struct sockaddr_storage *addr) {
    int sock, family;
    family = ((struct sockaddr *) addr)->sa_family;
    if ((sock = socket(family, SOCK_STREAM, 6)) == -1) {
        reportDebug("Failed to create socket for connection." + std::string(strerror(errno)), 1);
        return (-1);
    }
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 10;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
            &so_linger, sizeof (so_linger)) == -1) {
        reportDebug("Failed to set option to listening socket." + std::string(strerror(errno)), 1);
        return (-1);
   }
    if (connect(sock, (struct sockaddr *) addr, sizeof (*addr)) == -1) {
        reportDebug("Failed to connect to remote peer." + std::string(strerror(errno)) + MyAddr(*addr).get(), 1);
        return(-1);
    }
    MyAddr mad(*addr);
    reportDebug("Connected to " + mad.get(), 5);
    return (sock);
}

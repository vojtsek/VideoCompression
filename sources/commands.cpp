#include "headers/commands.h"
#include "headers/include_list.h"
#include "headers/handle_IO.h"
#include "headers/defines.h"

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


int32_t NetworkCommand::connectPeer(struct sockaddr_storage *addr) {
    int32_t sock, family;
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
        close(sock);
        return (-1);
   }
    if (connect(sock, (struct sockaddr *) addr, sizeof (*addr)) == -1) {
        reportDebug("Failed to connect to remote peer." + std::string(strerror(errno)) + MyAddr(*addr).get(), 1);
        close(sock);
        return(-1);
    }
    MyAddr mad(*addr);
    reportDebug("Connected to " + mad.get(), 5);
    return (sock);
}

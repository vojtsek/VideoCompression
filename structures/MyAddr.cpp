#include "headers/defines.h"
#include "headers/include_list.h"

#include <arpa/inet.h>

std::string MyAddr::get() {
    std::stringstream ss;
    // format the string
    ss << "Address: " << addr << " : " << port;
    return (ss.str());
}

void MyAddr::print() {
    reportStatus(get());
}

MyAddr::MyAddr(const struct sockaddr_storage &address): TTL(0) {
    if (((struct sockaddr_in *)(&address))->sin_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *) &address;
        // need to use ntohs
        port = ntohs(addr->sin_port);
        family = AF_INET;
    } else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &address;
        port = ntohs(addr->sin6_port);
        family = AF_INET6;
    }
    addr = networkHelper::storage2addrstr(
                address);
}

struct sockaddr_storage MyAddr::getAddress() {
    return networkHelper::addrstr2storage(
                addr.c_str(), port, family);
}

int64_t MyAddr::send(int64_t fd) {
    if (sendInt64(fd, family) == -1) {
        reportDebug("Failed to send family.", 1);
        return -1;
    }

    if (sendInt64(fd, port) == -1) {
        reportDebug("Failed to send port number.", 1);
        return -1;
    }

    if (sendInt64(fd, TTL) == -1) {
        reportDebug("Failed to send TTL.", 1);
        return -1;
    }

    if (sendString(fd, addr) == -1) {
        reportDebug("Failed to send address string.", 1);
        return -1;
    }

    return 0;
}

int64_t MyAddr::receive(int64_t fd) {
    if (receiveInt64(fd, family) == -1){
        reportDebug("Failed to receive family.", 1);
        return -1;
    }

    if (receiveInt64(fd, port) == -1){
        reportDebug("Failed to receive port number.", 1);
        return -1;
    }

    if (receiveInt64(fd, TTL) == -1){
        reportDebug("Failed to receive TTL.", 1);
        return -1;
    }

    if ((addr = receiveString(fd)).empty()) {
        reportDebug("Failed to receive address string.", 1);
        return -1;
    }

    return 0;
}

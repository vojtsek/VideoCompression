#include "headers/defines.h"
#include "headers/include_list.h"

#include <arpa/inet.h>

std::string MyAddr::get() {
    std::stringstream ss;
    ss << "Address: " << addr << " : " << port;
    return (ss.str());
}

void MyAddr::print() {
    reportStatus(get());
}

/*bool MyAddr::equalsTo(const sockaddr_storage &that) {
    return cmpStorages(this->addr, that);
}*/

MyAddr::MyAddr(const struct sockaddr_storage &address) {
    char adr4[sizeof (struct sockaddr_in)], adr6[sizeof (struct sockaddr_in6)];
    if (((struct sockaddr_in *)(&address))->sin_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *) &address;
        inet_ntop(AF_INET, &addr->sin_addr,
              adr4, sizeof(struct sockaddr_in));
        this->addr = std::string(adr4);
        port = ntohs(addr->sin_port);
    } else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &address;
        inet_ntop(AF_INET6, &addr->sin6_addr,
              adr6, sizeof(struct sockaddr_in6));
       this->addr = std::string(adr6);
        port = ntohs(addr->sin6_port);
    }
}

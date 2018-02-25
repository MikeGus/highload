#include "mysocket.h"
#include <arpa/inet.h>
#include <string.h>

int get_sockfd(const int port) {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return ERROR_SOCKET_CREATION;
    }

    int state = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*) &state, sizeof(state)) < 0) {
        return ERROR_SOCKET_SET_REUSEADD_OPTION;
    }

//    maybe use later to check perf
//    if (setsockopt(sockfd, IPROTO_TCP, TCP_CORK, (const void*) &state, sizeof(state)) < 0) {
//        return ERROR_SOCKET_TCP_CORK;
//    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);

    if (bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0) {
        return ERROR_SOCKET_BINDING;
    }

    if (listen(sockfd, LISTEN_QUEQUE_SIZE) < 0) {
        return ERROR_SOCKET_LISTEN;
    }

    return sockfd;
}

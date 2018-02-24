#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>


#define LISTEN_QUEQUE_SIZE 5
#define BUFFER_SIZE 256
#define DEFAULT_PORT 8000
#define PROC_NUMBER 8

#define ERROR_INVALID_DIRECTORY_ARGUMENT -1

#define ERROR_SOCKET_CREATION -2
#define ERROR_SOCKET_SET_REUSEADD_OPTION -3
#define ERROR_SOCKET_TCP_CORK -4
#define ERROR_SOCKET_BINDING -5
#define ERROR_SOCKET_LISTEN -6

#define ERROR_FORK -7

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

void process_request(const int fd, struct sockaddr_in* clientaddr) {
    return;
}


int main(int argc, char* argv[]) {

    int port = DEFAULT_PORT;

    if (argc > 1) {
        char* port_argument = argv[1];
        if (isdigit(*port_argument)) {
            port = atoi(port_argument);
        }
    }

    if (argc > 2) {
        char* work_dir_argument = argv[2];
        if (chdir(work_dir_argument) != 0) {
            fprintf(stderr, "Can't work with provided directory: %s\n", work_dir_argument);
            return ERROR_INVALID_DIRECTORY_ARGUMENT;
        }
    }

    int sockfd = get_sockfd(port);

    if (sockfd > 0) {
        printf("Sockets listening to port %d, sockfd is %d.\n", port, sockfd);
    } else {
        perror("Error with socket setup");
        return sockfd;
    }

    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in clientaddr;
    socklen_t clientaddr_length = sizeof(clientaddr);

    for (unsigned i = 0; i < PROC_NUMBER; ++i) {
        int pid = fork();
        if (pid == 0) {
            while (1) {
                int acceptfd = accept(sockfd, (struct sockaddr*) &clientaddr, &clientaddr_length);
                process_request(acceptfd, &clientaddr);
                close(acceptfd);
            }
        } else if (pid > 0) {
            printf("Succesfully forked. Child pid is %d\n", pid);
        } else {
            perror("Can't fork");
            return ERROR_FORK;
        }
    }

    while (1) {
        int acceptfd = accept(sockfd, (struct sockaddr*) &clientaddr, &clientaddr_length);
        process_request(acceptfd, &clientaddr);
        close(acceptfd);
    }

    return 0;
}

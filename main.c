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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sendfile.h>

#define BUFFER_SIZE 128
#define METHOD_BUFFER_SIZE 8
#define INDEX_HTML_LENGTH 10

#define MAX_ERROR_RESPONSE_LENGTH 512

#define DEFAULT_PORT 8000
#define PROC_NUMBER 8
#define LISTEN_QUEQUE_SIZE 5

#define ERROR_INVALID_DIRECTORY_ARGUMENT -1

#define ERROR_SOCKET_CREATION -2
#define ERROR_SOCKET_SET_REUSEADD_OPTION -3
#define ERROR_SOCKET_TCP_CORK -4
#define ERROR_SOCKET_BINDING -5
#define ERROR_SOCKET_LISTEN -6

#define ERROR_FORK -7

#define ERROR_READ_REQUEST -8
#define ERROR_PARSE_REQUEST -9
#define ERROR_INVALID_REQUEST_METHOD -10

#define ERROR_SEND_MSG -11

#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_HEAD "HEAD"

#define HTTP_STATUS_OK 200
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405
#define HTTP_STATUS_INTERNAL_SERVER_ERROR 500

struct response_status {
    unsigned short code;
    char* description;
};

struct http_request {
    char method[METHOD_BUFFER_SIZE];
    char location[BUFFER_SIZE + INDEX_HTML_LENGTH];    /*room for index.html*/
};

struct mime_map {
    const char* extention;
    const char* mime_type;
};

struct mime_map mime_types[] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".html", "text/html"},
    {".js", "application/javascript"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".swf", "application/x-shockwave-flash"},
    {NULL, NULL}
};

const char* default_mime_type = "text/html";

const char* get_mime_type(const char* path) {
    char* dot = strrchr(path, '.');
    if (dot) {
        struct mime_map* map = mime_types;
        while (map->extention != NULL) {
            if (strcmp(map->extention, dot) == 0) {
                return map->mime_type;
            }
            map++;
        }
    }
    return default_mime_type;
}

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

int parse_request(const int fd, struct http_request* request) {
    char buf[METHOD_BUFFER_SIZE + 1 + BUFFER_SIZE + INDEX_HTML_LENGTH];
    int length = read(fd, buf, BUFFER_SIZE + INDEX_HTML_LENGTH - 1);

    if (length < 0) {
        return ERROR_READ_REQUEST;
    }

    char buffer_location[BUFFER_SIZE + INDEX_HTML_LENGTH];

    char format[BUFFER_SIZE];
    sprintf(format, "%%%ds%%%ds", METHOD_BUFFER_SIZE - 1, BUFFER_SIZE - 11);

    if (sscanf(buf, format, request->method, buffer_location) < 2) {
        return ERROR_PARSE_REQUEST;
    } else {
        if (strcmp(request->method, HTTP_METHOD_GET) != 0
                && strcmp(request->method, HTTP_METHOD_HEAD) != 0) {
            return ERROR_INVALID_REQUEST_METHOD;
        }
    }

    int location_length = strlen(buffer_location);
    if (buffer_location[location_length - 1] == '/') {
        sprintf(buffer_location + location_length, "index.html");
    }

    sprintf(request->location, "%s", buffer_location + 1);

    return 0;
}


ssize_t reliable_write(const int fd, const char* msg, size_t size) {
    size_t left = size;
    ssize_t written = 0;

    char* msgptr = (char*) msg;

    while (left > 0) {
        written = write(fd, msgptr, left);
        if (written <= 0) {
            if (errno == EINTR) {
                written = 0;
            } else {
                return ERROR_SEND_MSG;
            }
        }
        left -= written;
        msgptr += written;
    }

    return size;
}

ssize_t reliable_sendfile(const int fd, const int rqfd, size_t size) {
    off_t offset = 0;
    while ((unsigned) offset < size) {
        if (sendfile(fd, rqfd, &offset, size) <= 0) {
            break;
        }
    }

    return offset;
}


void send_error(const int fd, const struct response_status* status, const char* msg) {
    char raw_responce[MAX_ERROR_RESPONSE_LENGTH];
    sprintf(raw_responce, "HTTP/1.1 %d %s\r\n", status->code, status->description);
    sprintf(raw_responce + strlen(raw_responce), "Content-length: %lu\r\n\r\n", strlen(msg));
    sprintf(raw_responce + strlen(raw_responce), "%s", msg);
    reliable_write(fd, raw_responce, strlen(raw_responce));
}

void send_static(const int fd, const int rqfd, const char* path,
                 const struct response_status* status, const size_t file_size,
                 const unsigned short headers_only) {
    char* content = (char*) calloc(file_size + BUFFER_SIZE, sizeof(char));
    sprintf(content, "HTTP/1.1 %u %s\r\nAccept-Ranges: bytes\r\n", status->code, status->description);
    sprintf(content + strlen(content), "Cache-control: no-cache\r\n");
    sprintf(content + strlen(content), "Content-length: %lu\r\n", file_size);
    const char* mime_type = get_mime_type(path);
    sprintf(content + strlen(content), "Content-type: %s\r\n\r\n", mime_type);

    reliable_write(fd, content, strlen(content));

    if (!headers_only) {
        reliable_sendfile(fd, rqfd, file_size);
    }

}

void process_request(const int fd) {
//    printf("Process %d accepted request for fd %d\n", getpid(), fd);
    struct http_request request;
    struct response_status response;

    int parse_status = parse_request(fd, &request);

    switch (parse_status) {
    case 0:
        response.code = HTTP_STATUS_OK;
        response.description = "Ok";
        break;
    case ERROR_INVALID_REQUEST_METHOD:
        response.code = HTTP_STATUS_METHOD_NOT_ALLOWED;
        response.description = "Method not allowed";
        break;
    case ERROR_PARSE_REQUEST:
        response.code = HTTP_STATUS_BAD_REQUEST;
        response.description = "Bad request";
    default:
        response.code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response.description = "Internal server error";
        break;
    }

    struct stat filestat;

    int rqfd = open(request.location, O_RDONLY, 0);
    if (rqfd < 0) {
        response.code = HTTP_STATUS_NOT_FOUND;
        response.description = "Not found";
    }

    fstat(rqfd, &filestat);
    if (response.code == HTTP_STATUS_OK && !S_ISREG(filestat.st_mode)) {
        response.code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response.description = "Internal server error";
    }

    if (response.code == HTTP_STATUS_OK) {
        unsigned short headers_only = 0;
        if (strcmp(request.method, HTTP_METHOD_HEAD) == 0) {
            headers_only = 1;
        }
        send_static(fd, rqfd, request.location, &response, filestat.st_size, headers_only);
    } else {
        char* error_msg;
        switch (response.code) {
        case HTTP_STATUS_BAD_REQUEST:
            error_msg = "Bad request";
            break;
        case HTTP_STATUS_METHOD_NOT_ALLOWED:
            error_msg = "Unsupported method";
            break;
        case HTTP_STATUS_NOT_FOUND:
            error_msg = "File not found";
            break;
        default:
            error_msg = "Unknown error";
            break;
        }
        send_error(fd, &response, error_msg);
    }
    close(rqfd);
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
        printf("Server is listening to port %d.\n", port);
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
                process_request(acceptfd);
                close(acceptfd);
            }
        } else if (pid > 0) {
//            printf("Succesfully forked. Child pid is %d\n", pid);
        } else {
            perror("Can't fork");
            return ERROR_FORK;
        }
    }

    while (1) {
        int acceptfd = accept(sockfd, (struct sockaddr*) &clientaddr, &clientaddr_length);
        process_request(acceptfd);
        close(acceptfd);
    }

    return 0;
}

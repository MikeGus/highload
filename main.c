#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netinet/ip.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "mime_types.h"
#include "http_request_parser.h"
#include "http_methods.h"
#include "http_statuses.h"
#include "autowrite.h"
#include "autosendfile.h"
#include "mysocket.h"
#include "parse_config.h"

#define MAX_ERROR_RESPONSE_LENGTH 256

#define DEFAULT_PORT 80
#define DEFAULT_CPU_NUMBER 4
#define DEFAULT_PATH "/var/www/html"

#define ERROR_FORK -1
#define ERROR_CWD -2

void form_basic_responce(char* content, const struct response_status* status) {

    sprintf(content, "HTTP/1.1 %u %s\r\n", status->code, status->description);
    sprintf(content + strlen(content), "Server: mikegus c server v1.0\r\n");

    char buf[BUFFER_SIZE];
    time_t now = time(NULL);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "%a, %d, %b, %Y %H:%M:%S %Z", &tm);

    sprintf(content + strlen(content), "Date: %s\r\n", buf);
    sprintf(content + strlen(content), "Connection: close\r\n");
}


void send_error(const int fd, const struct response_status* status) {
    char content[MAX_ERROR_RESPONSE_LENGTH];
    form_basic_responce(content, status);
    reliable_write(fd, content, strlen(content));
}

void send_static(const int fd, const int rqfd, const char* path,
                 const struct response_status* status, const size_t file_size,
                 const unsigned short headers_only) {

    char* content = (char*) calloc(file_size + BUFFER_SIZE, sizeof(char));
    form_basic_responce(content, status);

    sprintf(content + strlen(content), "Content-Length: %lu\r\n", file_size);
    const char* mime_type = get_mime_type(path);
    sprintf(content + strlen(content), "Content-Type: %s\r\n\r\n", mime_type);

    reliable_write(fd, content, strlen(content));

    if (!headers_only) {
        reliable_sendfile(fd, rqfd, file_size);
    }

    free(content);
}

void process_request(const int fd) {
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
        break;
    case ERROR_ESCAPING_ROOT:
        response.code = HTTP_STATUS_BAD_REQUEST;
        response.description = "Bad request";
        break;
    default:
        response.code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response.description = "Internal server error";
        break;
    }

    struct stat filestat;

    int rqfd = open(request.location, O_RDONLY, 0);
    if (rqfd < 0) {
        if (strstr(request.location, "index.html")) {
            response.code = HTTP_STATUS_FORBIDDEN;
            response.description = "Forbidden";
        } else {
            response.code = HTTP_STATUS_NOT_FOUND;
            response.description = "Not found";
        }

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
        send_error(fd, &response);
    }
    close(rqfd);
}


int main(void) {

    struct configf config;
    config.port = DEFAULT_PORT;
    config.cpu = DEFAULT_CPU_NUMBER;

    if (sprintf(config.path, "%s", DEFAULT_PATH) < 0) {
        perror("Sprintf error");
        return ERROR_SPRINTF;
    }

    int error = parse_config(&config);
    if (error != 0) {
        fprintf(stderr, "Can't parse %s\n", PATH);
        return error;
    }

    if (chdir(config.path) != 0) {
        perror("Can't change cwd");
        return ERROR_CWD;
    }

    int sockfd = get_sockfd(config.port);
    if (sockfd > 0) {
        printf("Server is listening to port %d.\nStatic files root: %s\n",
               config.port, config.path);
    } else {
        perror("Error with socket setup");
        return sockfd;
    }

    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in clientaddr;
    socklen_t clientaddr_length = sizeof(clientaddr);

    for (unsigned i = 1; i < config.cpu; ++i) {
        int pid = fork();
        if (pid == 0) {
            while (1) {
                int acceptfd = accept(sockfd, (struct sockaddr*) &clientaddr, &clientaddr_length);
                process_request(acceptfd);
                close(acceptfd);
            }
        } else if (pid < 0) {
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

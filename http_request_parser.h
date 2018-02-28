#ifndef HTTP_REQUEST_PARSER_H
#define HTTP_REQUEST_PARSER_H

#define BUFFER_SIZE 1024
#define METHOD_BUFFER_SIZE 8
#define INDEX_HTML_LENGTH 10

#define ERROR_READ_REQUEST -201
#define ERROR_PARSE_REQUEST -202
#define ERROR_INVALID_REQUEST_METHOD -203
#define ERROR_ESCAPING_ROOT -204


struct response_status {
    unsigned short code;
    char* description;
};

struct http_request {
    char method[METHOD_BUFFER_SIZE];
    char location[BUFFER_SIZE + INDEX_HTML_LENGTH];    /*room for index.html*/
};

int parse_request(const int fd, struct http_request* request);

#endif // HTTP_REQUEST_PARSER_H

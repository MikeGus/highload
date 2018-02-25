#include "http_request_parser.h"
#include "http_methods.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void get_path_without_parameters(char* src, char* dest, int max) {
    char* ptr = strchr(src, '?');
    if (ptr) {
        max = ptr - src;
    }
    strncpy(dest, src, max);
    dest[max] = '\0';
}

void decode_url(char* src, char* dest, int max) {
    char *p = src;
    char code[3] = {0};
    while (*p != '\0' && --max) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char) strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

int parse_request(const int fd, struct http_request* request) {
    char buf[METHOD_BUFFER_SIZE + 1 + BUFFER_SIZE + INDEX_HTML_LENGTH + 10];
    int length = read(fd, buf, METHOD_BUFFER_SIZE + 1 + BUFFER_SIZE + INDEX_HTML_LENGTH + 9);

    if (length < 0) {
        return ERROR_READ_REQUEST;
    }

    char buffer_location[BUFFER_SIZE + INDEX_HTML_LENGTH + 1];

    char format[BUFFER_SIZE];
    sprintf(format, "%%%ds%%%ds", METHOD_BUFFER_SIZE - 1, BUFFER_SIZE - INDEX_HTML_LENGTH - 1);

    if (sscanf(buf, format, request->method, buffer_location) < 2) {
        return ERROR_PARSE_REQUEST;
    } else if (strcmp(request->method, HTTP_METHOD_GET) != 0
               && strcmp(request->method, HTTP_METHOD_HEAD) != 0) {
        return ERROR_INVALID_REQUEST_METHOD;
    }

    if (strstr(buffer_location, "/..")) {
        return ERROR_ESCAPING_ROOT;
    }

    char raw_buffer_location[BUFFER_SIZE + INDEX_HTML_LENGTH + 1];
    get_path_without_parameters(buffer_location, raw_buffer_location, strlen(buffer_location));

    char decoded_buffer_location[BUFFER_SIZE + INDEX_HTML_LENGTH];
    decode_url(raw_buffer_location, decoded_buffer_location, BUFFER_SIZE + INDEX_HTML_LENGTH);

    int location_length = strlen(decoded_buffer_location);

    if (decoded_buffer_location[location_length - 1] == '/') {
        sprintf(decoded_buffer_location + location_length, "index.html");
    }

    sprintf(request->location, "%s", decoded_buffer_location + 1);

    return 0;
}

#include "autowrite.h"
#include <errno.h>
#include <unistd.h>

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

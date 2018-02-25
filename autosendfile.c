#include "autosendfile.h"
#include <sys/sendfile.h>

ssize_t reliable_sendfile(const int fd, const int rqfd, size_t size) {
    off_t offset = 0;
    while ((unsigned) offset < size) {
        if (sendfile(fd, rqfd, &offset, size) <= 0) {
            break;
        }
    }

    return offset;
}

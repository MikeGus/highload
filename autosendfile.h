#ifndef AUTOSENDFILE_H
#define AUTOSENDFILE_H
#include <sys/types.h>

ssize_t reliable_sendfile(const int fd, const int rqfd, size_t size);

#endif // AUTOSENDFILE_H

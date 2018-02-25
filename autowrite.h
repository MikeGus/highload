#ifndef AUTOWRITE_H
#define AUTOWRITE_H
#include <sys/types.h>

#define ERROR_SEND_MSG -12

ssize_t reliable_write(const int fd, const char* msg, size_t size);

#endif // AUTOWRITE_H

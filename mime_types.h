#ifndef MIME_TYPES_H
#define MIME_TYPES_H

#include <string.h>


struct mime_map {
    const char* extention;
    const char* mime_type;
};

const char* get_mime_type(const char* path);

#endif // MIME_TYPES_H

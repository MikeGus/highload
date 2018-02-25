#include "mime_types.h"

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

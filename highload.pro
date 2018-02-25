TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    mime_types.c \
    http_request_parser.c \
    autowrite.c \
    autosendfile.c \
    mysocket.c

HEADERS += \
    mime_types.h \
    http_request_parser.h \
    http_methods.h \
    http_statuses.h \
    autowrite.h \
    autosendfile.h \
    mysocket.h

DISTFILES += \
    Dockerfile

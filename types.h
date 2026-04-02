#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>

typedef struct {
    int id;
    char *country;
    char *city;
    char *provider;
    char *host;
} Server;

typedef struct {
    char country[128];
    Server server;
    int has_country;
    int has_server;
    int has_download;
    int has_upload;
    double download_mbps;
    double upload_mbps;
} TestResult;

typedef struct {
    Server *items;
    size_t count;
} ServerList;

#endif

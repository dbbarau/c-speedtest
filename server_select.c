#include "server_select.h"

#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

static size_t discard_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

static double probe_server_seconds(const char *host) {
    CURL *curl = curl_easy_init();
    if (!curl) return 1e9;

    char url[512];
    snprintf(url, sizeof(url), "http://%s", host);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);

    CURLcode rc = curl_easy_perform(curl);
    double total = 1e9;
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
    }
    curl_easy_cleanup(curl);
    return total;
}

static void copy_server(Server *dst, const Server *src) {
    *dst = *src;
}

int pick_best_server(const ServerList *list, const char *country, Server *out) {
    if (!list || !out || list->count == 0) return -1;

    int use_country = country && country[0] != '\0';
    double best = 1e9;
    const Server *best_server = NULL;

    for (size_t i = 0; i < list->count; i++) {
        const Server *s = &list->items[i];
        if (use_country && strcasecmp(s->country, country) != 0) continue;
        double t = probe_server_seconds(s->host);
        if (t < best) {
            best = t;
            best_server = s;
        }
    }

    if (!best_server && use_country) {
        for (size_t i = 0; i < list->count; i++) {
            const Server *s = &list->items[i];
            double t = probe_server_seconds(s->host);
            if (t < best) {
                best = t;
                best_server = s;
            }
        }
    }

    if (!best_server) return -1;
    copy_server(out, best_server);
    return 0;
}

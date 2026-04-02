#include "speedtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#define TEST_TIMEOUT_SECONDS 15L

typedef struct {
    const char *data;
    size_t len;
    size_t pos;
} UploadCtx;

static size_t discard_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb;
}

static size_t upload_read_cb(char *buffer, size_t size, size_t nitems, void *instream) {
    UploadCtx *ctx = (UploadCtx *)instream;
    size_t max = size * nitems;
    size_t remaining = ctx->len - ctx->pos;
    size_t to_copy = remaining < max ? remaining : max;
    if (to_copy > 0) {
        memcpy(buffer, ctx->data + ctx->pos, to_copy);
        ctx->pos += to_copy;
    }
    return to_copy;
}

static int calc_mbps(curl_off_t bytes, double seconds, double *out) {
    if (seconds <= 0.0 || !out) return -1;
    *out = ((double)bytes * 8.0) / (seconds * 1000000.0);
    return 0;
}

int run_download_test(const Server *server, double *mbps_out) {
    if (!server || !mbps_out) return -1;

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    char url[512];
    snprintf(url, sizeof(url), "http://%s/download?size=100000000", server->host);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    curl_off_t bytes = 0;
    double time_s = 0.0;
    if (rc == CURLE_OK || rc == CURLE_OPERATION_TIMEDOUT) {
        curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &bytes);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time_s);
    }
    curl_easy_cleanup(curl);

    if (bytes <= 0 || calc_mbps(bytes, time_s, mbps_out) != 0) return -1;
    return 0;
}

int run_upload_test(const Server *server, double *mbps_out) {
    if (!server || !mbps_out) return -1;

    size_t payload_size = 32 * 1024 * 1024;
    char *payload = (char *)malloc(payload_size);
    if (!payload) return -1;
    memset(payload, 'A', payload_size);

    UploadCtx ctx = { .data = payload, .len = payload_size, .pos = 0 };

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(payload);
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "http://%s/upload.php", server->host);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, upload_read_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)payload_size);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    curl_off_t bytes = 0;
    double time_s = 0.0;
    if (rc == CURLE_OK || rc == CURLE_OPERATION_TIMEDOUT) {
        curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD_T, &bytes);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time_s);
    }

    curl_easy_cleanup(curl);
    free(payload);

    if (bytes <= 0 || calc_mbps(bytes, time_s, mbps_out) != 0) return -1;
    return 0;
}

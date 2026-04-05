#include "speedtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#define TEST_TIMEOUT_SECONDS 15L

static size_t discard_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr;
    (void)userdata;
    return size * nmemb;
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
    snprintf(url, sizeof(url), "http://%s/download?size=25000000", server->host);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);

    curl_off_t bytes = 0;
    double time_s = 0.0;
    long http_code = 0;

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &bytes);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time_s);

    curl_easy_cleanup(curl);

    if (rc != CURLE_OK && rc != CURLE_OPERATION_TIMEDOUT) return -1;
    if (http_code != 200) return -1;
    if (bytes <= 0 || calc_mbps(bytes, time_s, mbps_out) != 0) return -1;
    return 0;
}

int run_upload_test(const Server *server, double *mbps_out) {
    if (!server || !mbps_out) return -1;

    size_t payload_size = 25 * 1024 * 1024;
    char *payload = (char *)malloc(payload_size);
    if (!payload) return -1;
    memset(payload, 'A', payload_size);

    CURL *curl = curl_easy_init();
    if (!curl) {
        free(payload);
        return -1;
    }

    char url[512];
    snprintf(url, sizeof(url), "http://%s/upload", server->host);

    struct curl_httppost *form = NULL;
    struct curl_httppost *last = NULL;
    curl_formadd(&form, &last,
                 CURLFORM_COPYNAME, "file",
                 CURLFORM_BUFFER, "payload",
                 CURLFORM_BUFFERPTR, payload,
                 CURLFORM_BUFFERLENGTH, payload_size,
                 CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_cb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TEST_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode rc = curl_easy_perform(curl);
    curl_off_t bytes = 0;
    double time_s = 0.0;

    if (rc == CURLE_OK || rc == CURLE_OPERATION_TIMEDOUT) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            curl_formfree(form);
            curl_easy_cleanup(curl);
            free(payload);
            return -1;
        }
        curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD_T, &bytes);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time_s);
    } else {
        curl_formfree(form);
        curl_easy_cleanup(curl);
        free(payload);
        return -1;
    }

    curl_formfree(form);
    curl_easy_cleanup(curl);
    free(payload);

    if (bytes <= 0 || calc_mbps(bytes, time_s, mbps_out) != 0) return -1;
    return 0;
}
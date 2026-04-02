#include "geo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>
typedef struct {
    char *data;
    size_t len;
} Buffer;

static size_t write_to_buffer(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    Buffer *b = (Buffer *)userdata;
    char *n = realloc(b->data, b->len + total + 1);
    if (!n) return 0;
    b->data = n;
    memcpy(b->data + b->len, ptr, total);
    b->len += total;
    b->data[b->len] = '\0';
    return total;
}

static int fetch_json_country(const char *url, const char *field, char *out, int out_size) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    Buffer b = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "teltonika-speedtest/1.0");

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK || code < 200 || code >= 300 || !b.data) {
        free(b.data);
        return -1;
    }

    cJSON *root = cJSON_Parse(b.data);
    free(b.data);
    if (!root) return -1;

    cJSON *country = cJSON_GetObjectItemCaseSensitive(root, field);
    if (!cJSON_IsString(country) || !country->valuestring) {
        cJSON_Delete(root);
        return -1;
    }

    snprintf(out, (size_t)out_size, "%s", country->valuestring);
    cJSON_Delete(root);
    return 0;
}

int detect_country(char *country_out, int out_size) {
    if (!country_out || out_size <= 0) return -1;

    if (fetch_json_country("https://ipapi.co/json/", "country_name", country_out, out_size) == 0) return 0;
    if (fetch_json_country("http://ip-api.com/json/", "country", country_out, out_size) == 0) return 0;
    return -1;
}

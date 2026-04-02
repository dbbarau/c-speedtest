#include "json_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

static char *xstrdup(const char *s) {
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

static char *dup_json_string(cJSON *obj, const char *key) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(item) || !item->valuestring) {
        return xstrdup("");
    }
    return xstrdup(item->valuestring);
}

int load_server_list(const char *path, ServerList *out) {
    if (!out) return -1;
    out->items = NULL;
    out->count = 0;

    char *text = read_file(path);
    if (!text) return -1;

    cJSON *root = cJSON_Parse(text);
    free(text);
    if (!cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return -1;
    }

    size_t count = cJSON_GetArraySize(root);
    Server *items = (Server *)calloc(count, sizeof(Server));
    if (!items) {
        cJSON_Delete(root);
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        cJSON *obj = cJSON_GetArrayItem(root, (int)i);
        items[i].id = cJSON_GetObjectItemCaseSensitive(obj, "id") ? cJSON_GetObjectItemCaseSensitive(obj, "id")->valueint : -1;
        items[i].country = dup_json_string(obj, "country");
        items[i].city = dup_json_string(obj, "city");
        items[i].provider = dup_json_string(obj, "provider");
        items[i].host = dup_json_string(obj, "host");
    }

    cJSON_Delete(root);
    out->items = items;
    out->count = count;
    return 0;
}

void free_server_list(ServerList *list) {
    if (!list || !list->items) return;
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].country);
        free(list->items[i].city);
        free(list->items[i].provider);
        free(list->items[i].host);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

const Server *find_server_by_id(const ServerList *list, int id) {
    if (!list) return NULL;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].id == id) return &list->items[i];
    }
    return NULL;
}

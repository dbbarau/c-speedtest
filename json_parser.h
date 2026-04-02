#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "types.h"

int load_server_list(const char *path, ServerList *out);
void free_server_list(ServerList *list);
const Server *find_server_by_id(const ServerList *list, int id);

#endif

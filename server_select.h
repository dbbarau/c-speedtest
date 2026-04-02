#ifndef SERVER_SELECT_H
#define SERVER_SELECT_H

#include "types.h"

int pick_best_server(const ServerList *list, const char *country, Server *out);

#endif

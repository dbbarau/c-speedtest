#ifndef SPEEDTEST_H
#define SPEEDTEST_H

#include "types.h"

int run_download_test(const Server *server, double *mbps_out);
int run_upload_test(const Server *server, double *mbps_out);

#endif

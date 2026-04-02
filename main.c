#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "geo.h"
#include "json_parser.h"
#include "server_select.h"
#include "speedtest.h"
#include "types.h"

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("  -a, --auto                Run full automated test\n");
    printf("  -g, --geo                 Detect user country\n");
    printf("  -b, --best-server         Pick best server\n");
    printf("  -d, --download            Run download test\n");
    printf("  -u, --upload              Run upload test\n");
    printf("  -s, --server-id <id>      Server id for download/upload\n");
    printf("  -f, --file <path>         Server list JSON path (default: speedtest_server_list.json)\n");
    printf("  -h, --help                Show this help\n");
}

static void print_result(const TestResult *r) {
    if (r->has_country) printf("Vartotojo vietove: %s\n", r->country);
    if (r->has_server) {
        printf("Serveris: id=%d, host=%s, provider=%s, country=%s, city=%s\n",
               r->server.id, r->server.host, r->server.provider, r->server.country, r->server.city);
    }
    if (r->has_download) printf("Parsisiuntimo greitis: %.2f Mbps\n", r->download_mbps);
    if (r->has_upload) printf("Issiuntimo greitis: %.2f Mbps\n", r->upload_mbps);
}

int main(int argc, char **argv) {
    int do_auto = 0, do_geo = 0, do_best = 0, do_download = 0, do_upload = 0;
    int server_id = -1;
    const char *json_file = "speedtest_server_list.json";

    static struct option long_opts[] = {
        {"auto", no_argument, 0, 'a'},
        {"geo", no_argument, 0, 'g'},
        {"best-server", no_argument, 0, 'b'},
        {"download", no_argument, 0, 'd'},
        {"upload", no_argument, 0, 'u'},
        {"server-id", required_argument, 0, 's'},
        {"file", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "agbdus:f:h", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'a': do_auto = 1; break;
            case 'g': do_geo = 1; break;
            case 'b': do_best = 1; break;
            case 'd': do_download = 1; break;
            case 'u': do_upload = 1; break;
            case 's': server_id = atoi(optarg); break;
            case 'f': json_file = optarg; break;
            case 'h':
            default: print_usage(argv[0]); return opt == 'h' ? 0 : 1;
        }
    }

    if (!do_auto && !do_geo && !do_best && !do_download && !do_upload) {
        print_usage(argv[0]);
        return 1;
    }

    if ((do_download || do_upload) && !do_auto && server_id < 0) {
        fprintf(stderr, "Klaida: download/upload testui reikia --server-id arba naudokite --auto.\n");
        return 1;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "Klaida: nepavyko inicializuoti curl.\n");
        return 1;
    }

    ServerList list = {0};
    TestResult result = {0};

    if (load_server_list(json_file, &list) != 0) {
        fprintf(stderr, "Klaida: nepavyko nuskaityti serveriu failo: %s\n", json_file);
        curl_global_cleanup();
        return 1;
    }

    if (do_auto || do_geo || do_best) {
        printf("[INFO] Nustatoma vietove...\n");
        if (detect_country(result.country, sizeof(result.country)) == 0) {
            result.has_country = 1;
            if (!do_auto && do_geo && !do_best && !do_download && !do_upload) {
                print_result(&result);
            }
        } else {
            fprintf(stderr, "[WARN] Nepavyko nustatyti vietoves.\n");
        }
    }

    if (do_auto || do_best) {
        printf("[INFO] Parenkamas geriausias serveris...\n");
        if (pick_best_server(&list, result.has_country ? result.country : NULL, &result.server) == 0) {
            result.has_server = 1;
            if (!do_auto && do_best && !do_download && !do_upload) {
                print_result(&result);
            }
        } else {
            fprintf(stderr, "Klaida: nepavyko parinkti serverio.\n");
            free_server_list(&list);
            curl_global_cleanup();
            return 1;
        }
    }

    if ((do_download || do_upload) && !do_auto) {
        const Server *s = find_server_by_id(&list, server_id);
        if (!s) {
            fprintf(stderr, "Klaida: serveris su id=%d nerastas.\n", server_id);
            free_server_list(&list);
            curl_global_cleanup();
            return 1;
        }
        result.server = *s;
        result.has_server = 1;
    }

    if (do_auto || do_download) {
        printf("[INFO] Vykdomas parsisiuntimo testas (iki 15s)...\n");
        if (run_download_test(&result.server, &result.download_mbps) == 0) {
            result.has_download = 1;
        } else {
            fprintf(stderr, "[WARN] Parsisiuntimo testas nepavyko.\n");
        }
    }

    if (do_auto || do_upload) {
        printf("[INFO] Vykdomas issiuntimo testas (iki 15s)...\n");
        if (run_upload_test(&result.server, &result.upload_mbps) == 0) {
            result.has_upload = 1;
        } else {
            fprintf(stderr, "[WARN] Issiuntimo testas nepavyko.\n");
        }
    }

    print_result(&result);

    free_server_list(&list);
    curl_global_cleanup();
    return 0;
}

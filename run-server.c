#include <errno.h> // perror
#include <netdb.h> // getaddrinfo

#include "common.h"
#include "utils.h"

#define PRINT_USAGE() \
    fprintf(stderr, "USAGE: %s server-address [port]", argv[0]);

char const *
arg_or_default(unsigned argc, char *argv[], unsigned arg,
               char const *default_value) {
    return arg < argc ? argv[arg] : default_value;
}

int
main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        PRINT_USAGE();
        exit(EXIT_FAILURE);
    }
    unsigned arg            = 0;
    char const *server_node = argv[++arg];
    char const *port        = arg_or_default(argc, argv, ++arg, DEFAULT_PORT);

    struct cmn_peer *SCOPED_PEER server = cmn_peer_create(server_node, port);
    if (server == NULL) {
        fprintf(stderr, "can't create server\n");
        exit(EXIT_FAILURE);
    }
    printf("listening\n");
    if (cmn_listen(server) == -1) {
        printf("stopped listening\n");
    }
}

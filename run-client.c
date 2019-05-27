#include <assert.h> // assert

#include "common.h"
#include "utils.h"

#define PRINT_USAGE()                                                       \
    fprintf(stderr, "USAGE: %s message server-address [port]", argv[0]);

char const *
arg_or_default(unsigned argc, char *argv[], unsigned arg,
               char const *default_value) {
    return arg < argc ? argv[arg] : default_value;
}

int
main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        PRINT_USAGE();
        exit(EXIT_FAILURE);
    }
    unsigned arg = 0;
    char const *message = argv[++arg];
    char const *server_node = argv[++arg];
    char const *port = arg_or_default(argc, argv, ++arg, DEFAULT_PORT);

    struct cmn_peer *SCOPED_PEER client = cmn_peer_create(server_node, port);
    char buf[PACKET_SIZE + 1];
    size_t buf_len;
    int rc = cmp_exchange(client, message, buf, &buf_len);
    if (rc == -1) {
        fprintf(stderr, "exchange failed\n");
        exit(EXIT_FAILURE);
    }
    assert(buf_len < PACKET_SIZE + 1);
    buf[buf_len] = '\0';

    printf("sent: '%s', received: '%s'\n", message, buf);
}

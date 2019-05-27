#include "common.h"

#include <netdb.h>   // getaddrinfo, getnameinfo
#include <assert.h>  // assert
#include <poll.h>    // pollfd
#include <unistd.h>  // close
#include <stdbool.h> // bool
#include <stdlib.h>  // calloc
#include <string.h>  // strerror
#include <errno.h>   // errno
#include <stddef.h>  // NULL, size_t

#include "utils.h"

struct cmn_peer {
    int sfd;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
};

struct cmn_peer *
cmn_peer_create(char const *node, char const *port) {
    struct addrinfo hints = {.ai_family = AF_UNSPEC,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = IPPROTO_TCP,
            .ai_flags = AI_NUMERICSERV};
    struct addrinfo *result;
    int gai_ec = getaddrinfo(node, port, &hints, &result);
    if (gai_ec != 0) {
        err_log("getaddrinfo failed: %s", gai_strerror(gai_ec));
        return NULL;
    }

    struct cmn_peer *peer = calloc(1, sizeof(struct cmn_peer));
    if (peer == NULL) {
        err_log("calloc failed: %s", strerror(errno));
        free(peer);
        return NULL;
    }
    bool socket_ok = false;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        peer->sfd = socket(rp->ai_family, rp->ai_socktype | SOCK_NONBLOCK, rp->ai_protocol);
        if (peer->sfd != -1) {
            socket_ok = true;
            memcpy(&peer->peer_addr, rp->ai_addr, rp->ai_addrlen);
            peer->peer_addr_len = rp->ai_addrlen;
            break;
        }
    }
    freeaddrinfo(result);
    if (!socket_ok) {
        err_log("could not create socket");
        free(peer);
        return NULL;
    }

    return peer;
}

void
cmn_peer_destroy(struct cmn_peer *peer) {
    if (peer == NULL) {
        return;
    }
    close(peer->sfd);
    free(peer);
}

int
cmn_listen(struct cmn_peer *server) {
    int rc;
    rc = bind(server->sfd, (struct sockaddr const *) &server->peer_addr, server->peer_addr_len);
    if (rc == -1) {
        err_log("bind failed: %s", strerror(errno));
        return -1;
    }
    listen(server->sfd, 2048);

    int open_max = get_open_max();
    if (open_max == -1) {
        return -1;
    }
    size_t pfds_cap = open_max;
    assert(pfds_cap >= 1);
    size_t pfds_len = 1;
    struct pollfd *SCOPED_MEM pfds = malloc(pfds_cap * sizeof(struct pollfd));
    if (pfds == NULL) {
        err_log("malloc failed");
        return -1;
    }
    pfds[0] = (struct pollfd) {.fd = server->sfd, .events = POLLIN};
    for (size_t i = 1; i < pfds_cap; ++i) {
        pfds[i].fd = -1;
    }

    char buf[PACKET_SIZE + 1];
    ssize_t nread;
    while (true) {
        rc = poll(pfds, pfds_len, -1);
        if (rc == -1) {
            err_log("poll failed: %s", strerror(errno));
            return -1;
        }
        if (pfds[0].revents & POLLIN) {
            int connfd = accept(server->sfd, NULL, NULL);
            if (connfd == -1) {
                printf("failed to accept connection, continuing…\n");
            } else {
                size_t i;
                for (i = 1; i < pfds_cap; ++i) {
                    if (pfds[i].fd < 0) {
                        pfds[i] = (struct pollfd) {.fd = connfd, .events = POLLIN};
                        break;
                    }
                }
                if (i == pfds_cap) {
                    printf("can't accept any more clients\n");
                } else {
                    pfds_len = MAX(pfds_len, i + 1);
                }
            }
        }
        for (size_t i = 1; i < pfds_len; ++i) {
            if (pfds[i].fd < 0) {
                continue;
            }
            if (pfds[i].revents & POLLHUP) {
                printf("client closed its end of channel\n");
                close(pfds[i].fd);
                pfds[i].fd = -1;
            } else if (pfds[i].revents & POLLNVAL) {
                printf("invalid request, fd not open\n");
                pfds[i].fd = -1;
            } else if (pfds[i].revents & POLLERR) {
                printf("error condition\n");
                close(pfds[i].fd);
                pfds[i].fd = -1;
            } else if (pfds[i].revents == POLLIN) {
                nread = read(pfds[i].fd, buf, PACKET_SIZE);
                if (nread == -1) {
                    printf("read failed: %s\n", strerror(errno));
                    close(pfds[i].fd);
                    pfds[i].fd = -1;
                    continue;
                }
                buf[nread] = '\0';
                printf("received: %s", buf);
                write(pfds[i].fd, buf, nread); // echo!
                close(pfds[i].fd);
                pfds[i].fd = -1;
            }
        }
    }
}

int
cmp_exchange(struct cmn_peer *client, char const *message, char buf[static PACKET_SIZE], size_t *buf_len) {
    int rc;
    rc = connect(client->sfd, (struct sockaddr const *) &client->peer_addr, client->peer_addr_len);
    struct pollfd pfds[] = {{.fd = client->sfd, .events = POLLOUT}};
    if (rc == -1) {
        if (errno != EINPROGRESS) {
            fprintf(stderr, "failed to connect: %s\n", strerror(errno));
            return -1;
        }
        printf("waiting for connect completion\n");
        while (true) {
            rc = poll(pfds, 1, 0);
            if (rc == -1) {
                fprintf(stderr, "poll failed: %s\n", strerror(errno));
                return -1;
            }
            assert(rc == 1);
            if (pfds[0].revents & POLLHUP) {
                printf("server closed its end of channel\n");
                return -1;
            } else if (pfds[0].revents & POLLNVAL) {
                printf("invalid request, fd not open\n");
                return -1;
            } else if (pfds[0].revents & POLLERR) {
                printf("error condition\n");
                return -1;
            }
            if (pfds[0].revents & POLLOUT) {
                int error = 0;
                socklen_t err_len = sizeof(error);
                if (getsockopt(client->sfd, SOL_SOCKET, SO_ERROR, &error, &err_len) == -1) {
                    printf("getsockopt failed: %s\n", strerror(errno));
                    return -1;
                }
                if (error == 0) {
                    printf("connected!\n");
                    sleep(3);
                    rc = write(client->sfd, message, strlen(message) + 1);
                    if (rc == -1) {
                        printf("write failed: %s\n", strerror(errno));
                    }
                    break;
                }
            }
        }
    }
    pfds[0].events = POLLIN;
    rc = poll(pfds, 1, -1);
    if (rc == -1) {
        fprintf(stderr, "poll failed: %s\n", strerror(errno));
        return -1;
    }
    assert(rc == 1);
    while (true) {
        if (pfds[0].revents & POLLHUP) {
            printf("server closed its end of channel\n");
            return -1;
        }
        if (pfds[0].revents & POLLNVAL) {
            printf("invalid request, fd not open\n");
            return -1;
        }
        if (pfds[0].revents & POLLERR) {
            printf("error condition\n");
            return -1;
        }
        if (pfds[0].revents & POLLIN) {
            ssize_t nread;
            nread = read(pfds[0].fd, buf, PACKET_SIZE);
            if (nread == -1) {
                fprintf(stderr, "read failed: %s\n", strerror(errno));
                return -1;
            }
            *buf_len = nread;
            return 0;
        }
    }
}

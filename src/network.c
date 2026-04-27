#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "history.h"
#include "network.h"
#include "query.h"

#if !defined(TPL_BACKEND_BAREMETAL)
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

int net_domain_connect(const char *name, bool udp)
{
    return -1;
}

int net_domain_server(const char *name, bool udp)
{
    return -1;
}

int net_connect(const char *hostname, unsigned port, bool udp, bool nodelay)
{
    return -1;
}

int net_server(const char *hostname, unsigned port, bool udp, const char *keyfile,
               const char *certfile)
{
    return -1;
}

int net_accept(stream *str)
{
    return -1;
}

void net_set_nonblocking(stream *str)
{
}

void *net_enable_ssl(int fd, const char *hostname, bool is_server, int level, const char *certfile)
{
    return NULL;
}

size_t net_write(const void *ptr, size_t nbytes, stream *str)
{
    return 0;
}

int net_peekc(stream *str)
{
    return -1;
}

int net_getc(stream *str)
{
    return -1;
}

size_t net_read(void *ptr, size_t len, stream *str)
{
    return 0;
}

int net_getline(char **lineptr, size_t *n, stream *str)
{
    return -1;
}

int net_close(stream *str)
{
    return -1;
}

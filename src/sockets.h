
#ifndef SOCKETS_H
#define SOCKETS_H

#include "threads.h"

extern int create_udp_socket(struct ThreadData *tdata);
extern int destroy_udp_socket(struct ThreadData *tdata);

#endif

#ifndef __SOCKETS_H__
#define __SOCKETS_H__

#include "threads.h"

extern int create_udp_socket(struct ThreadData *tdata);
extern int destroy_udp_socket(struct ThreadData *tdata);

#endif
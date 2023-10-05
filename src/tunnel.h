
#ifndef TUNNEL_H
#define TUNNEL_H

extern void *tunnel_read_tap_handler(void *arg);

extern void *tunnel_write_socket_handler(void *arg);

extern void *tunnel_read_socket_handler(void *arg);

extern void *tunnel_write_tap_handler(void *arg);

#endif
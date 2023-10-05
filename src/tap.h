
#ifndef TAP_H
#define TAP_H

#include "threads.h"

extern void create_tap_interface(char *ifname, struct ThreadData *tdata);
extern void destroy_tap_interface(struct ThreadData *tdata);
extern int start_tap_interface(struct ThreadData *tdata);
extern int set_interface_mtu(struct ThreadData *tdata);

#endif
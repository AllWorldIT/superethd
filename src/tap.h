
#ifndef __TAP_H__
#define __TAP_H__

#include "threads.h"

extern void create_tap_interface(char *ifname, struct ThreadData *tdata);
extern void destroy_tap_interface(struct ThreadData *tdata);
extern int start_tap_interface(struct ThreadData *tdata);
extern int set_interface_mtu(struct ThreadData *tdata);
// FIXME: remove
extern void disable_interface_ipv6(const char *ifname);

#endif
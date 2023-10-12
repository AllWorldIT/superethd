
#pragma once

#include "threads.hpp"

#include <string>

void create_tap_interface(const std::string ifname, struct ThreadData *tdata);
void destroy_tap_interface(struct ThreadData *tdata);
int start_tap_interface(struct ThreadData *tdata);
int set_interface_mtu(struct ThreadData *tdata);

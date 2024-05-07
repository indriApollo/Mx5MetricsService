//
// Created by rleroux on 4/28/24.
//

#ifndef MX5METRICSSERVICE_SERVER_H
#define MX5METRICSSERVICE_SERVER_H

#include "metrics.h"

int setup_server_socket(const char *socket_name);

void close_server_socket(int fd, const char *socket_name);

int handle_incoming_server_msg(int fd, const struct metrics *metrics);

#endif //MX5METRICSSERVICE_SERVER_H

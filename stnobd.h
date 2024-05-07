//
// Created by rleroux on 5/4/24.
//

#ifndef MX5METRICSSERVICE_STNOBD_H
#define MX5METRICSSERVICE_STNOBD_H

#define STNOBD_CFG_DISABLE_ECHO   "ATE0\r"
#define STNOBD_CFG_ENABLE_HEADER  "ATH1\r"
#define STNOBD_CFG_DISABLE_SPACES "ATS0\r"

#define STNOBD_CFG_FILTER(hex_str_id) "STFPA" hex_str_id ",FFF\r"

#include "metrics.h"
#include <termios.h>
#include <stdbool.h>

struct stnobd_context {
    int fd;
    bool reset_in_progress;
    bool must_configure;
    bool in_monitoring_mode;
    char **cfg_cmds;
    int cfg_cmds_count;
    int current_cfg_cmd;
};

int setup_stnobd(const char *port_name, speed_t baud_rate,
                 char **cfg_cmds, int cfg_cmds_count, struct stnobd_context *ctx);

void close_stnobd(struct stnobd_context *ctx);

int handle_incoming_stnobd_msg(struct stnobd_context *ctx, struct metrics *metrics);

int send_stnobd_reset_cmd(struct stnobd_context *ctx);

#endif //MX5METRICSSERVICE_STNOBD_H

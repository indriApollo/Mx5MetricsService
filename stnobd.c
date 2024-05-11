//
// Created by rleroux on 5/4/24.
//

#include "stnobd.h"
#include "serial_port.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define WAIT_FOR_FIRST_BYTE 1
#define INTER_BYTE_TIMEOUT  1
#define CFG_ACK_LEN         4 // >OK\r

static void msleep(int milliseconds) {
    const struct timespec ts = { .tv_nsec = milliseconds * 1000000, .tv_sec = 0 };
    nanosleep(&ts, NULL);
}

static int send_cfg_cmd(struct stnobd_context *ctx) {
    assert(ctx->current_cfg_cmd < ctx->cfg_cmds_count);
    const char *cmd = ctx->cfg_cmds[ctx->current_cfg_cmd];
    const size_t cmd_len = strlen(cmd);

    printf("sending cfg cmd %.*s\n", (int)strlen(cmd) - 1 /* omit \r */, cmd);

    ssize_t c = write(ctx->fd, cmd, cmd_len);
    if (c < 0) {
        perror("write send_cfg_cmd");
        return -1;
    }

    if (c != cmd_len) {
        fprintf(stderr, "incomplete write cfg cmd (actual %zd expected %zu)\n", c, cmd_len);
        return -1;
    }

    return 0;
}

static int start_monitoring_mode(struct stnobd_context *ctx) {
    const char cmd[] = "STM\r";
    const size_t cmd_len = strlen(cmd);

    // Get rid of any existing unwanted bytes
    tcflush(ctx->fd, TCIOFLUSH);
    ssize_t c = write(ctx->fd, cmd, strlen(cmd));
    if (c < 0) {
        perror("write start_monitoring_mode");
        return -1;
    }

    if (c != cmd_len) {
        fprintf(stderr, "incomplete write stm cmd (actual %zd expected %zu)\n", c, cmd_len);
        return -1;
    }

    ctx->in_monitoring_mode = true;

    return 0;
}

static int stop_monitoring_mode(struct stnobd_context *ctx) {
    const char cmd[] = "\r";
    const size_t cmd_len = strlen(cmd);

    ssize_t c = write(ctx->fd, cmd, strlen(cmd));
    if (c < 0) {
        perror("write stop_monitoring_mode");
        return -1;
    }

    if (c != cmd_len) {
        fprintf(stderr, "incomplete write stm cmd (actual %zd expected %zu)\n", c, cmd_len);
        return -1;
    }

    ctx->in_monitoring_mode = false;

    return 0;
}

static int handle_monitoring_rsp(struct stnobd_context *ctx, struct metrics *metrics) {
    uint16_t can_id;
    uint64_t can_data;

    if (ctx->mon_rsp_pos >= MONITORING_RSP_LEN) {
        ctx->mon_rsp_pos = 0;
    }

    int n = MONITORING_RSP_LEN - ctx->mon_rsp_pos;

    ssize_t c = read(ctx->fd, ctx->mon_rsp_buf + ctx->mon_rsp_pos, n);
    if (c < 0) {
        perror("read handle_monitoring_rsp");
        return -1;
    }

    //printf("got rsp %zd, '%.*s'\n", c, (int)(c - 1), ctx->mon_rsp_buf + ctx->mon_rsp_pos);

    // Remember our current progress
    ctx->mon_rsp_pos += c;

    if (c != n) {
        printf("partial rsp (got %zd expected %d)\n", c, n);
        // No worries, try to get the rest with the next read
        return 1;
    }

    assert(ctx->mon_rsp_pos == MONITORING_RSP_LEN);

    int rsp_last_index = MONITORING_RSP_LEN - 1;

    // Handle misaligned reads
    if (ctx->mon_rsp_buf[rsp_last_index] != '\r') {
        printf("expected monitoring msg to end with \\r\n");

        char *cr = strchr(ctx->mon_rsp_buf, '\r');
        if (cr == NULL)
            // We couldn't find \r, let the next read happen normally in the hopes to finally get it
            return 1;

        int cr_index = (int)(cr - ctx->mon_rsp_buf);
        int after_cr_len = rsp_last_index - cr_index;

        // Move everything after \r to the start of the buffer
        // and advance pos
        memmove(ctx->mon_rsp_buf, cr + 1, after_cr_len);
        ctx->mon_rsp_pos = after_cr_len;

        return 1;
    }

    char can_id_str[CAN_ID_STR_LEN + 1 /* null terminator */] = {0};
    memcpy(can_id_str, ctx->mon_rsp_buf, CAN_ID_STR_LEN);
    can_id = strtoul(can_id_str, NULL, 16);

    char can_data_str[CAN_DATA_STR_LEN + 1 /* null terminator */] = {0};
    memcpy(can_data_str, ctx->mon_rsp_buf + CAN_ID_STR_LEN, CAN_DATA_STR_LEN);
    can_data = strtoull(can_data_str, NULL, 16);

    return handle_can_msg(can_id, can_data, metrics);
}

static int handle_cfg_rsp(struct stnobd_context *ctx) {
    char buf[CFG_ACK_LEN + 1 /* null terminator */] = {0};

    msleep(100); // Wait for full response (> prompt char can lag behind initial ack chars)
    ssize_t c = read(ctx->fd, buf, CFG_ACK_LEN);
    if (c < 0) {
        perror("read handle_cfg_rsp");
        return -1;
    }

    // TODO: Retry
    if (strstr(buf, "OK") == NULL) {
        printf("didnt get expected cfg ack %zd, %s\n", c, buf);
    }
    // Move to next cfg cmd
    else {
        ctx->current_cfg_cmd++;
        if (ctx->current_cfg_cmd >= ctx->cfg_cmds_count)
        {
            ctx->must_configure = false;
            printf("all cfg cmds done\n");
            start_monitoring_mode(ctx);
            return 0;
        }
    }

    tcflush(ctx->fd, TCIFLUSH);
    return send_cfg_cmd(ctx);
}

static int handle_reset_rsp(struct stnobd_context *ctx) {
    // TODO : the startup msg might be chopped when reading and we'd miss it

    const char startup_msg[] = "ELM327";
    const int startup_msg_len = strlen(startup_msg);
    char buf[32] = {0};

    msleep(100); // Wait for full response (> prompt char can lag behind initial startup msg chars)
    // Read a bunch of bytes in the hope of finding the STN startup msg
    ssize_t c = read(ctx->fd, buf, sizeof(buf) - 1 /* null terminator */);
    if (c < 0) {
        perror("read handle_reset_rsp");
        return -1;
    }

    if (c < startup_msg_len) {
        printf("not enough bytes to contain STN startup msg\n");
        return 0;
    }

    if (strstr(buf, startup_msg) != NULL)
    {
        // We got the STN startup message, reset is complete
        ctx->reset_in_progress = false;
        // Get rid of whatever might be left in the serial input buffer
        tcflush(ctx->fd, TCIFLUSH);
        printf("STN reset done\n");

        send_cfg_cmd(ctx);
    }

    return 0;
}

int setup_stnobd(const char *port_name, speed_t baud_rate,
                 char **cfg_cmds, int cfg_cmds_count, struct stnobd_context *ctx) {
    int fd = open_serial_port_blocking_io(port_name);
    if (fd < 0) return -1;

    if (set_serial_port_access_exclusive(fd) < 0) return -1;

    if (configure_serial_port(fd, WAIT_FOR_FIRST_BYTE, INTER_BYTE_TIMEOUT, baud_rate) < 0) {
        set_serial_port_access_nonexclusive(fd);
        return -1;
    }

    ctx->fd = fd;
    ctx->reset_in_progress = false;
    ctx->must_configure = false;
    ctx->cfg_cmds = cfg_cmds;
    ctx->cfg_cmds_count = cfg_cmds_count;
    ctx->current_cfg_cmd = 0;

    return fd;
}

void close_stnobd(struct stnobd_context *ctx) {
    if (ctx->in_monitoring_mode) stop_monitoring_mode(ctx);
    set_serial_port_access_nonexclusive(ctx->fd);
    close(ctx->fd);
}

int handle_incoming_stnobd_msg(struct stnobd_context *ctx, struct metrics *metrics)
{
    if (ctx->reset_in_progress)
        return handle_reset_rsp(ctx);

    if (ctx->must_configure)
        return handle_cfg_rsp(ctx);

    if (ctx->in_monitoring_mode)
        return handle_monitoring_rsp(ctx, metrics);

    // TODO
    char buf[255] = {0};
    ssize_t c = read(ctx->fd, buf, 255 - 1);
    tcflush(ctx->fd, TCIFLUSH);
    printf("unknown stn msg (%zd bytes) %s\n", c, buf);

    return 0;
}

int send_stnobd_reset_cmd(struct stnobd_context *ctx) {
    const char cmd[] = "ATZ\r";
    size_t cmd_len = strlen(cmd);

    // Get rid of any existing unwanted bytes
    tcflush(ctx->fd, TCIOFLUSH);
    ssize_t c = write(ctx->fd, cmd, cmd_len);
    if (c < 0) {
        perror("write send_stnobd_reset_cmd");
        return -1;
    }

    if (c != cmd_len) {
        fprintf(stderr, "incomplete write reset cmd (actual %zd expected %zu)\n", c, cmd_len);
        return -1;
    }

    ctx->reset_in_progress = true;
    ctx->must_configure = true;

    printf("STN reset in progress\n");
    return 0;
}

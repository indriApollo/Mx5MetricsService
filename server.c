//
// Created by rleroux on 4/28/24.
//

#include "server.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <string.h>
#include "commands.h"

#define REC_BUFFER_SIZE CMD_ID_SIZE
#define RSP_BUFFER_SIZE CMD_RSP_MAX_SIZE

int setup_server_socket(const char *socket_name)
{
    int fd;
    struct sockaddr_un addr;

    unlink(socket_name);

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("socket");
        return -1;
    }

    strcpy(addr.sun_path, socket_name);
    addr.sun_family = AF_UNIX;

    if (bind(fd, (struct sockaddr *) &addr,
             strlen(addr.sun_path) + sizeof (addr.sun_family)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    return fd;
}

void close_server_socket(int fd, const char *socket_name) {
    close(fd);
    unlink(socket_name);
}

int handle_incoming_server_msg(int fd, const struct metrics *metrics)
{
    uint8_t rec_buffer[REC_BUFFER_SIZE];
    uint8_t rsp_buffer[RSP_BUFFER_SIZE];

    struct sockaddr_un client_address;
    socklen_t client_len = sizeof(struct sockaddr_un);

    ssize_t rec_count = recvfrom(fd, rec_buffer, REC_BUFFER_SIZE, 0,
                         (struct sockaddr *) &client_address, &client_len);

    if (rec_count < 0) {
        perror("recvfrom");
        return -1;
    }

    printf("Got %zd bytes from %s\n", rec_count, client_address.sun_path);

    if (rec_count < REC_BUFFER_SIZE) {
        printf("Datagram too small");
        return 0;
    }

    printf("cmd id %d %s\n", rec_buffer[0], command_str(rec_buffer[0]));

    size_t rsp_len = handle_command(rec_buffer[0], metrics, rsp_buffer);

    if (sendto(fd, rsp_buffer, rsp_len, 0,
               (const struct sockaddr *) &client_address, client_len) < 0) {
        perror("sendto");
        return -1;
    }

    return 0;
}
